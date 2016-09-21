/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>

#include "uart.h"
#include "dbg.h"

#define UART_RX_BUFFER_SIZE	1024	/* XXX: Reduce this later on? */
#define CALLBACK_TRIGGER_MARK	((uint16_t)(UART_RX_BUFFER_SIZE * ALMOST_FULL_FRAC))

#define CEIL(x, y)		(((x) + (y) - 1) / (y))
#define TIMEOUT_BYTE_US		555	/* One character width in us @ 115200 bps. */
#define TIMEOUT_MS_NCHAR(x)	CEIL((x) * TIMEOUT_BYTE_US, 1000)

static uart_rx_cb rx_cb;	/* Receive callback */
#define INVOKE_CALLBACK(x)	if (rx_cb) rx_cb((x))

#define BAUD_RATE		115200

/* The internal buffer that will hold incoming data. */
static volatile struct {
	uint8_t buffer[UART_RX_BUFFER_SIZE];
	uint16_t ridx;
	uint16_t widx;
	uint16_t num_unread;
} rx;

static volatile uint8_t hal_rx_buffer;	/* Receive buffer for the HAL. */
#define HAL_RX_BUF_SIZE		((uint8_t)1)

/* Store the idle timeout in number of characters. By default, it is 5. */
static uint16_t timeout = TIMEOUT_MS_NCHAR(5);

/* Store the time when the last byte was received. */
static volatile uint32_t last_rx_byte_timestamp;

/* Set when the first byte of the response arrives. */
static volatile bool resp_start;

static UART_HandleTypeDef comm_uart;

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	/*
	 * Configure the pins for USART2:
	 * CTS : PD3
	 * RTS : PD4
	 * TX  : PD5
	 * RX  : PD6
	 */
	GPIO_InitTypeDef uart_pins;
	__HAL_RCC_GPIOD_CLK_ENABLE();
	uart_pins.Pin = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6;
	uart_pins.Mode = GPIO_MODE_AF_PP;
	uart_pins.Pull = GPIO_PULLUP;
	uart_pins.Speed = GPIO_SPEED_FREQ_HIGH;
	uart_pins.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(GPIOD, &uart_pins);

	/* Set up the UART IRQ handler. */
	HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(USART2_IRQn);
}

bool uart_module_init(void)
{
	/* Reset internal buffer's read and write heads. */
	rx.ridx = 0;
	rx.widx = 0;
	rx.num_unread = 0;

	/* Initialize USART2 in asynchronous mode with HW flow control. */
	__HAL_RCC_USART2_CLK_ENABLE();
	comm_uart.Instance = USART2;
	comm_uart.Init.BaudRate = BAUD_RATE;
	comm_uart.Init.WordLength = UART_WORDLENGTH_8B;
	comm_uart.Init.StopBits = UART_STOPBITS_1;
	comm_uart.Init.Parity = UART_PARITY_NONE;
	comm_uart.Init.Mode = UART_MODE_TX_RX;
	comm_uart.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
	/*
	 * Choose oversampling by 16 to increase tolerance of the receiver to
	 * noise on an asynchronous line.
	 */
	comm_uart.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&comm_uart) != HAL_OK)
		return false;


	/* Enable Error Interrupts: (Frame error, noise error, overrun error) */
	SET_BIT(comm_uart.Instance->CR3, USART_CR3_EIE);

	/* Enable the UART Parity Error and Data Register not empty Interrupts */
	SET_BIT(comm_uart.Instance->CR1, USART_CR1_PEIE | USART_CR1_RXNEIE);

	return true;
}

bool uart_tx(uint8_t *data, uint16_t size, uint16_t timeout_ms)
{
	if (HAL_UART_Transmit(&comm_uart, data, size, timeout_ms) != HAL_OK)
		return false;
	return true;
}

void uart_set_rx_callback(uart_rx_cb cb)
{
	rx_cb = cb;
}

void uart_config_idle_timeout(uint8_t t)
{
	timeout = TIMEOUT_MS_NCHAR(t);
}

int uart_read(uint8_t *buf, uint16_t buf_sz)
{
	/*
	 * Return an error if:
	 * 	> There are no bytes to read in the buffer.
	 * 	> If the buffer size exceeds the number of unread bytes.
	 */
	if (rx.num_unread == 0 ||
			buf_sz > rx.num_unread)
		return INV_DATA;

	/*
	 * Copy bytes into the supplied buffer and perform
	 * the necessary book-keeping.
	 */
	uint16_t n_bytes = buf_sz;
	uint16_t i = 0;
	while (buf_sz) {
		buf[i++] = rx.buffer[rx.ridx];
		rx.ridx = (rx.ridx + 1) % UART_RX_BUFFER_SIZE;
		rx.num_unread--;
		buf_sz--;
	}
	return n_bytes;
}

void uart_detect_recv_idle(void)
{
	/*
	 * Invoke the callback with the receive event in two situations:
	 * 	> When the RX line is detected to be idle after a response has
	 * 	  begun arriving.
	 * 	> When a certain percentage of bytes have been received.
	 */
	if ((resp_start && (HAL_GetTick() - last_rx_byte_timestamp > timeout)) ||
			(rx.num_unread == CALLBACK_TRIGGER_MARK)) {
		resp_start = false;
		INVOKE_CALLBACK(UART_EVENT_RESP_RECV);
	}
}

unsigned int uart_rx_available(void)
{
	return rx.num_unread;
}

void USART2_IRQHandler(void)
{
	uint32_t err_flags = 0x00;
	uint32_t uart_sr_reg = comm_uart.Instance->SR;
	err_flags = uart_sr_reg & (uint32_t)(USART_SR_PE | USART_SR_FE
			| USART_SR_ORE | USART_SR_NE);

	/* If any errors, clear it by reading the RX data register. */
	if (err_flags) {
		volatile uint8_t data = comm_uart.Instance->DR;
		UNUSED(data);
		return;
	}

	/* Store data into the read buffer. */
	resp_start = true;
	last_rx_byte_timestamp = HAL_GetTick();
	rx.buffer[rx.widx] = comm_uart.Instance->DR;
	rx.widx = (rx.widx + 1) % UART_RX_BUFFER_SIZE;
	rx.num_unread++;

	/* Check for buffer overflow. */
	if (rx.num_unread >= UART_RX_BUFFER_SIZE)
		INVOKE_CALLBACK(UART_EVENT_RX_OVERFLOW);
}

void uart_flush_rx_buffer(void)
{
	rx.widx = 0;
	rx.ridx = 0;
	rx.num_unread = 0;
}
