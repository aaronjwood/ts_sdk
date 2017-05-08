/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>	/* XXX: Remove after rewriting raise_err */

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
	/* XXX: Replace with routines from the GPIO HAL */
	gpio_config_t err_led;
	pin_name_t pin_name = PB7;
	err_led.dir = OUTPUT;
	err_led.pull_mode = PP_NO_PULL;
	err_led.speed = SPEED_LOW;
	gpio_init(pin_name, &err_led);

	while (1) {
		gpio_write(pin_name, PIN_LOW);
		sys_delay(1000);
		gpio_write(pin_name, PIN_HIGH);
	}
}
