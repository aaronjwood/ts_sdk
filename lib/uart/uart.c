/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>

#include "uart.h"
#include "dbg.h"

#define UART_RX_BUFFER_SIZE	1024	/* XXX: Might reduce this later on. */
#define CALLBACK_TRIGGER_MARK	(UART_RX_BUFFER_SIZE * ALMOST_FULL_FRAC)U

#define CEIL(x, y)		(((x) + (y) - 1) / (y))
#define TIMEOUT_BYTE_US		555	/* One character width in us @ 115200 bps. */
#define TIMEOUT_MS_NCHAR(x)	CEIL((x) * TIMEOUT_BYTE_US, 1000)

static uart_rx_cb rx_cb;	/* Receive callback */
#define INVOKE_CALLBACK(x)	if (rx_cb) rx_cb((x));

/* The internal buffer that will hold incoming data. */
static volatile struct _rx_buffer {
	uint8_t buffer[UART_RX_BUFFER_SIZE];
	uint16_t ridx;
	uint16_t widx;
} rx_buffer;

/* Store the idle timeout in number of characters. By default, it is 5. */
static uint8_t timeout = TIMEOUT_MS_NCHAR(5);

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
	rx_buffer.ridx = 0;
	rx_buffer.widx = 0;

	return true;
}

bool uart_rx_available(void)
{
	return false;
}

int uart_read(void)
{
	return INV_DATA;
}

bool uart_tx(uint8_t *data, uint16_t size, uint16_t timeout_ms)
{
	return false;
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
