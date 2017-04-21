/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>	/* XXX: Remove after rewriting raise_err */

#include <stdint.h>
#include "uart_hal.h"
#include "ts_sdk_config.h"
#include "dbg.h"

#define DBG_UART_TIMEOUT_MS	2000

static periph_t uart;

bool __dbg_module_init(void)
{
	const struct uart_pins pins = {
		.tx = DEBUG_UART_TX_PIN,
		.rx = DEBUG_UART_RX_PIN,
		.rts = DEBUG_UART_RTS_PIN,
		.cts = DEBUG_UART_CTS_PIN
	};

	const uart_config config = {
		.baud = DEBUG_UART_BAUD_RATE,
		.data_width = DEBUG_UART_DATA_WIDTH,
		.parity = DEBUG_UART_PARITY,
		.stop_bits = DEBUG_UART_STOP_BITS,
	};
	uart = uart_init(&pins, &config);
	if (uart == NO_PERIPH)
		return false;
	return true;
}

/*
 * The standard C library function printf calls _write to transmit individual
 * characters over the debug UART port.
 */
__attribute__((used))
ssize_t _write(int fd, const void *buf, size_t count)
{
	if (!uart_tx(uart, (uint8_t *)buf, count, DBG_UART_TIMEOUT_MS))
		return 0;
	return count;
}

void raise_err(void)
{
	/* XXX: Replace with routines from the GPIO HAL */
	__HAL_RCC_GPIOB_CLK_ENABLE();
	GPIO_InitTypeDef err_led;
	err_led.Pin = GPIO_PIN_7;
	err_led.Mode = GPIO_MODE_OUTPUT_PP;
	err_led.Pull = GPIO_NOPULL;
	err_led.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &err_led);

	while (1) {
		HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_7);
		HAL_Delay(1000);
	}
}
