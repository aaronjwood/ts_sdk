/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>

#include "uart.h"
#include "dbg.h"

#define UART_RX_BUFFER_SIZE	1024	/* XXX: Reduce this later on? */
#define CALLBACK_TRIGGER_MARK	(UART_RX_BUFFER_SIZE * ALMOST_FULL_FRAC)U

#define CEIL(x, y)		(((x) + (y) - 1) / (y))
#define TIMEOUT_BYTE_US		555	/* One character width in us @ 115200 bps. */
#define TIMEOUT_MS_NCHAR(x)	CEIL((x) * TIMEOUT_BYTE_US, 1000)

static uart_rx_cb rx_cb;	/* Receive callback */
#define INVOKE_CALLBACK()	if (rx_cb) rx_cb()

#define BAUD_RATE		115200

/* The internal buffer that will hold incoming data. */
static volatile struct {
	uint8_t buffer[UART_RX_BUFFER_SIZE];
	uint16_t ridx;
	uint16_t widx;
} rx;

static volatile uint8_t hal_rx_buffer;	/* Receive buffer for the HAL. */
#define HAL_RX_BUF_SIZE		((uint8_t)1)

/* Store the idle timeout in number of characters. By default, it is 5. */
static uint16_t timeout = TIMEOUT_MS_NCHAR(5);

/* Store the time when the last byte was received. */
static volatile uint32_t last_rx_byte_timestamp;

static UART_HandleTypeDef comm_uart;

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
	/*
	 * Configure the pins for USART1:
	 * TX  : PA9
	 * RX  : PA10
	 * CTS : PA11
	 * RTS : PA12
	 */
	GPIO_InitTypeDef uart_pins;
	__HAL_RCC_GPIOA_CLK_ENABLE();
	uart_pins.Pin = GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
	uart_pins.Mode = GPIO_MODE_AF_PP;
	uart_pins.Pull = GPIO_PULLUP;
	uart_pins.Speed = GPIO_SPEED_FREQ_HIGH;
	uart_pins.Alternate = GPIO_AF7_USART1;
	HAL_GPIO_Init(GPIOA, &uart_pins);

	/* Set up the UART IRQ handler. */
	HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(USART1_IRQn);
}

bool uart_module_init(void)
{
	rx.ridx = 0;
	rx.widx = 0;

	__HAL_RCC_USART1_CLK_ENABLE();
	comm_uart.Instance = USART1;
	comm_uart.Init.BaudRate = BAUD_RATE;
	comm_uart.Init.WordLength = UART_WORDLENGTH_8B;
	comm_uart.Init.StopBits = UART_STOPBITS_1;
	comm_uart.Init.Parity = UART_PARITY_NONE;
	comm_uart.Init.Mode = UART_MODE_TX_RX;
	comm_uart.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
	/*
	 * Choose oversampling by 16 to increase tolerance of the receiver to
	 * clock deviations.
	 */
	comm_uart.Init.OverSampling = UART_OVERSAMPLING_16;

	if (HAL_UART_Init(&comm_uart) != HAL_OK)
		return false;

	if (HAL_UART_Receive_IT(&comm_uart, (unsigned char *)&hal_rx_buffer,
				HAL_RX_BUF_SIZE) != HAL_OK)
		return false;

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

void USART1_IRQHandler(void)
{
	HAL_UART_IRQHandler(&comm_uart);
}

bool uart_rx_available(void)
{
	return !(rx.ridx == rx.widx);
}

int uart_read(void)
{
	if (rx.ridx == rx.widx)
		return INV_DATA;
	uint8_t data = rx.buffer[rx.ridx];
	rx.ridx = (rx.ridx + 1) % UART_RX_BUFFER_SIZE;
	return data;
}

void uart_handle_idle_timeout(void)
{
	if (HAL_GetTick() - last_rx_byte_timestamp >= timeout)
		INVOKE_CALLBACK();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	last_rx_byte_timestamp = HAL_GetTick();
	uint16_t space_full;
	/* TODO: Store received bytes. */
}
