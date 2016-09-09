/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include "dbg.h"
#include <stdint.h>

#define DBG_UART_TIMEOUT_MS	2000

static GPIO_InitTypeDef dbg_uart_pins;
static UART_HandleTypeDef dbg_uart;

bool __dbg_module_init(void)
{
	/* Use UART4 on PORTC (pins 10 & 11) as debug UART. */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_UART4_CLK_ENABLE();
	dbg_uart_pins.Pin = GPIO_PIN_10;	/* Configure TX only. */
	dbg_uart_pins.Mode = GPIO_MODE_AF_PP;
	dbg_uart_pins.Pull = GPIO_PULLUP;
	dbg_uart_pins.Speed = GPIO_SPEED_FREQ_LOW;
	dbg_uart_pins.Alternate = GPIO_AF8_UART4;
	HAL_GPIO_Init(GPIOC, &dbg_uart_pins);

	dbg_uart.Instance = UART4;
	dbg_uart.Init.BaudRate = 9600;
	dbg_uart.Init.WordLength = UART_WORDLENGTH_8B;
	dbg_uart.Init.StopBits = UART_STOPBITS_1;
	dbg_uart.Init.Parity = UART_PARITY_NONE;
	dbg_uart.Init.Mode = UART_MODE_TX;
	dbg_uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	dbg_uart.Init.OverSampling = UART_OVERSAMPLING_8;
	if (HAL_UART_Init(&dbg_uart) != HAL_OK)
		return false;

	return true;
}

/*
 * The standard C library function printf calls _write to transmit individual
 * characters over the debug UART port.
 */
ssize_t _write(int fd, const void *buf, size_t count)
{
	if (HAL_UART_Transmit(&dbg_uart,
				(uint8_t *)buf,
				count,
				DBG_UART_TIMEOUT_MS) != HAL_OK)
		return 0;
	return count;
}
