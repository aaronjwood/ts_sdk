/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>
#include "uart_hal.h"
#include "board_config.h"
#include "dbg.h"
#include "gpio_hal.h"
#include "sys.h"

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
	gpio_config_t err_led;
	err_led.dir = OUTPUT;
	err_led.pull_mode = PP_NO_PULL;
	err_led.speed = SPEED_LOW;
	gpio_init(ERROR_LED_PIN, &err_led);

	while (1) {
		gpio_write(ERROR_LED_PIN, PIN_LOW);
		sys_delay(1000);
		gpio_write(ERROR_LED_PIN, PIN_HIGH);
		sys_delay(1000);
	}
}
