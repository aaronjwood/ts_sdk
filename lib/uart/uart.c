/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include <stm32f4xx_hal.h>
#include "uart.h"

#define UART_RX_BUFFER_SIZE	1024	/* XXX: Reduce this later on? */
#define BAUD_RATE		115200
#define CALLBACK_TRIGGER_MARK	((buf_sz)(UART_RX_BUFFER_SIZE * ALMOST_FULL_FRAC))
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
	buf_sz ridx;
	buf_sz widx;
	buf_sz num_unread;
} rx;

/* Store the idle timeout in number of characters. */
static uint16_t timeout_chars;

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
		INVOKE_CALLBACK(UART_EVENT_RECVD_BYTES);
	}
}

bool uart_module_init(bool flow_ctrl, uint8_t t)
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

	if (t == 0)
		return false;
	timeout_chars = t;

	/* Initialize the idle timer. */
	if (tim_module_init() == false)
		return false;

	return true;
}

int uart_read(uint8_t *buf, buf_sz sz)
{
	/*
	 * Return an error if there are no bytes to read in the buffer or if a
	 * null pointer was supplied in place of the buffer.
	 */
	if (rx.num_unread == 0)
		return 0;

	if (!buf)
		return UART_INV_PARAM;

	/*
	 * Copy bytes into the supplied buffer and perform the necessary
	 * book-keeping.
	 */
	buf_sz n_bytes = (sz > rx.num_unread) ? rx.num_unread : sz;
	buf_sz i = 0;
	while ((n_bytes - i) > 0) {
		buf[i++] = rx.buffer[rx.ridx];
		rx.ridx = (rx.ridx + 1) % UART_RX_BUFFER_SIZE;
		rx.num_unread--;
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

/*
 * Find the substring 'substr' inside the receive buffer between the index
 * supplied and write index. Return the starting position of the substring if
 * found. Otherwise, return -1.
 */
static int find_substr_in_ring_buffer(buf_sz idx_start, uint8_t *substr, buf_sz nlen)
{
	if (!substr || nlen == 0)
		return -1;

	buf_sz bidx = idx_start;
	buf_sz idx = 0;
	buf_sz found_idx = idx_start;
	bool first_char_seen = false;
	do {
		if (substr[idx] == rx.buffer[bidx]) {	/* Bytes match */
			if (!first_char_seen) {
				first_char_seen = true;
				found_idx = bidx;
			}
			idx++;
			bidx++;
		} else {				/* Bytes mismatch */
			if (first_char_seen) {
				first_char_seen = false;
				bidx = found_idx + 1;
			} else
				bidx++;
			found_idx = idx_start;
			idx = 0;
		}
		if (first_char_seen && idx == nlen)	/* Substring found */
			return found_idx;
		if (bidx == UART_RX_BUFFER_SIZE)	/* Index wrapping */
			bidx = 0;
	} while(bidx != rx.widx);			/* Scan until write index */
	return -1;					/* No substring found */
}

int uart_line_avail(char *header, char *trailer)
{
	if (!trailer)
		return UART_INV_PARAM;

	if (rx.num_unread == 0)
		return 0;

	int hidx = rx.ridx;		/* Store the header index */
	int tidx = 0;			/* Store the trailer index */
	buf_sz len = 0;
	buf_sz hlen = strlen(header);
	buf_sz tlen = strlen(trailer);

	if (header) {
		/* Search for the header from where we left off reading. */
		hidx = find_substr_in_ring_buffer(rx.ridx, header, hlen);
		if (hidx == -1)
			return 0;	/* Header specified but not found. */
	}

	/* If the header was found, skip the length of the header from the
	 * read location. This workaround is for the case where the header and
	 * the trailer are the same.
	 */
	tidx = find_substr_in_ring_buffer(rx.ridx + hlen, trailer, tlen);
	if (tidx == -1)			/* Trailer not found. */
		return 0;

	len = ((tidx >= hidx) ? (tidx - hidx) : (tidx + UART_RX_BUFFER_SIZE - hidx));
	/* If there is a line, signified by (len != 0), adjust the length to
	 * include the trailer.
	 */
	len = (len == 0) ? len : len + tlen;
	return len;
}

void uart_flush_rx_buffer(void)
{
	rx.widx = 0;
	rx.ridx = 0;
	rx.num_unread = 0;
}

bool uart_tx(uint8_t *data, buf_sz size, uint16_t timeout_ms)
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

buf_sz uart_rx_available(void)
{
	return rx.num_unread;
}

buf_sz uart_get_rx_buf_size(void)
{
	return UART_RX_BUFFER_SIZE;
}

void TIM2_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&idle_timer);
}
