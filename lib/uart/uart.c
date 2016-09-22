/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include "uart.h"

#define UART_RX_BUFFER_SIZE	1024	/* XXX: Reduce this later on? */
#define BAUD_RATE		115200
#define CALLBACK_TRIGGER_MARK	((uint16_t)(UART_RX_BUFFER_SIZE * ALMOST_FULL_FRAC))
#define DEFAULT_IDLE_CHARS	5
#define ALMOST_FULL_FRAC	0.6	/* Call the receive callback once this
					 * fraction of the buffer is full.
					 */
#define CEIL(x, y)		(((x) + (y) - 1) / (y))
#define MICRO_SEC_MUL		1000000
#define TIM_BASE_FREQ_HZ	1000000
#define TIMEOUT_BYTE_US		CEIL((8 * MICRO_SEC_MUL), BAUD_RATE)

#define USART_IRQ_PRIORITY	5
#define TIM_IRQ_PRIORITY	6	/* TIM2 priority is lower than USART2 */

/* The internal buffer that will hold incoming data. */
static volatile struct {
	uint8_t buffer[UART_RX_BUFFER_SIZE];
	uint16_t ridx;
	uint16_t widx;
	uint16_t num_unread;
} rx;

/* Store the idle timeout in number of characters. */
static uint16_t timeout_chars = DEFAULT_IDLE_CHARS;

static uart_rx_cb rx_cb;	/* Receive callback */
#define INVOKE_CALLBACK(x)	if (rx_cb) rx_cb((x))

static volatile uint16_t num_idle_chars;
static volatile bool byte_recvd;	/* Set if a byte was received in the last
					 * TIMEOUT_BYTE_US microseconds.
					 */

static UART_HandleTypeDef comm_uart;
static TIM_HandleTypeDef idle_timer;

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

	/* Set up the USART IRQ handler. */
	HAL_NVIC_SetPriority(USART2_IRQn, USART_IRQ_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(USART2_IRQn);
}

static bool tim_module_init(void)
{
	/*
	 * Initialize the timer (TIM2) to have a period equal to the time it
	 * takes to receive a character at the current baud rate.
	 * TIM2's clock source is connected to APB1. According to the TRM,
	 * this source needs to be multiplied by 2 if the APB1 prescaler is not
	 * 1 (it's 4 in our case). Therefore effectively, the clock source for
	 * TIM2 is:
	 * APB1 x 2 = SystemCoreClock / 4 * 2 = SystemCoreClock / 2 = 90 MHz.
	 * The base frequency of the timer's clock is chosen as 1 MHz (time
	 * period of 1 microsecond).
	 * Therefore, prescaler = 90 MHz / 1 MHz - 1 = 89.
	 * The clock is set to restart every 70us (value of 'Period' member)
	 * which is how much time it takes to receive a character on a 115200 bps
	 * UART.
	 */
	__HAL_RCC_TIM2_CLK_ENABLE();
	idle_timer.Instance = TIM2;
	idle_timer.Init.Prescaler = SystemCoreClock / 2 / TIM_BASE_FREQ_HZ - 1;
	idle_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
	idle_timer.Init.Period = TIMEOUT_BYTE_US - 1;
	idle_timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	if (HAL_TIM_Base_Init(&idle_timer) != HAL_OK)
		return false;

	/* Enable the TIM2 interrupt. */
	HAL_NVIC_SetPriority(TIM2_IRQn, TIM_IRQ_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);

	return true;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/*
	 * If a byte was not received in the last timeout period, increment the
	 * number of idle characters seen so far. Otherwise, reset the number of
	 * idle characters.
	 */
	if (byte_recvd) {
		byte_recvd = false;
		num_idle_chars = 0;
	} else {
		num_idle_chars++;
	}

	/*
	 * Invoke the callback with the receive event in two situations:
	 * 	> When the RX line is detected to be idle after a response has
	 * 	  begun arriving.
	 * 	> When a certain percentage of bytes have been received.
	 */
	if ((num_idle_chars >= timeout_chars) ||
			(rx.num_unread == CALLBACK_TRIGGER_MARK)) {
		HAL_TIM_Base_Stop_IT(&idle_timer);
		INVOKE_CALLBACK(UART_EVENT_RESP_RECVD);
	}
}

bool uart_module_init(bool flow_ctrl)
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
	comm_uart.Init.HwFlowCtl = (flow_ctrl) ?
		UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
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

	/* Initialize the idle timer. */
	if (tim_module_init() == false)
		return false;

	return true;
}

int uart_read(uint8_t *buf, uint16_t sz)
{
	/*
	 * Return an error if:
	 * 	> There are no bytes to read in the buffer.
	 * 	> If the read size exceeds the number of unread bytes.
	 */
	if (rx.num_unread == 0 || sz > rx.num_unread || !buf)
		return UART_READ_ERR;

	/*
	 * Copy bytes into the supplied buffer and perform the necessary
	 * book-keeping.
	 */
	uint16_t n_bytes = sz;
	uint16_t i = 0;
	while (sz) {
		buf[i++] = rx.buffer[rx.ridx];
		rx.ridx = (rx.ridx + 1) % UART_RX_BUFFER_SIZE;
		rx.num_unread--;
		sz--;
	}
	return n_bytes;
}

void USART2_IRQHandler(void)
{
	uint32_t uart_sr_reg = comm_uart.Instance->SR;
	uint32_t err_flags = uart_sr_reg & (uint32_t)(USART_SR_PE | USART_SR_FE
			| USART_SR_ORE | USART_SR_NE);

	/* If any errors, clear it by reading the RX data register. */
	if (err_flags) {
		volatile uint8_t data = comm_uart.Instance->DR;
		UNUSED(data);
		return;
	}

	byte_recvd = true;

	/* If the timer isn't running, this is the first byte of the response. */
	if (!(idle_timer.Instance->CR1 & TIM_CR1_CEN)) {
		num_idle_chars = 0;
		HAL_TIM_Base_Start_IT(&idle_timer);
	}

	/* Buffer characters as long as the size of the buffer isn't exceeded. */
	if (rx.num_unread < UART_RX_BUFFER_SIZE) {
		rx.buffer[rx.widx] = comm_uart.Instance->DR;
		rx.widx = (rx.widx + 1) % UART_RX_BUFFER_SIZE;
		rx.num_unread++;
	} else {
		INVOKE_CALLBACK(UART_EVENT_RX_OVERFLOW);
	}
}

void uart_flush_rx_buffer(void)
{
	rx.widx = 0;
	rx.ridx = 0;
	rx.num_unread = 0;
}

bool uart_tx(uint8_t *data, uint16_t size, uint16_t timeout_ms)
{
	if (!data)
		return false;

	if (HAL_UART_Transmit(&comm_uart, data, size, timeout_ms) != HAL_OK)
		return false;
	return true;
}

void uart_set_rx_callback(uart_rx_cb cb)
{
	rx_cb = cb;
}

bool uart_config_idle_timeout(uint8_t t)
{
	if (t == 0)
		return false;
	timeout_chars = t;
	return true;
}

uint16_t uart_rx_available(void)
{
	return rx.num_unread;
}

void TIM2_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&idle_timer);
}
