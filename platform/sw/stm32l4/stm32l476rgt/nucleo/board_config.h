/**
 * \file Configures the debug port and other board specific pin / peripheral
 * mappings.
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \details \n
 * Target board : Nucleo-L415RGT \n
 * Target MCU   : STM32L476RGT
 */
#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/* If the MCU runs on an external clock / crystal, define its frequency */
#define EXT_CLK_FREQ_HZ		((uint32_t)8000000)

/* Pin and peripheral configuration for the debug UART */
#define DEBUG_UART_TX_PIN	PC10
#define DEBUG_UART_RX_PIN	NC
#define DEBUG_UART_RTS_PIN	NC
#define DEBUG_UART_CTS_PIN	NC
#define DEBUG_UART_BAUD_RATE	115200
#define DEBUG_UART_DATA_WIDTH	8
#define DEBUG_UART_PARITY	NONE
#define DEBUG_UART_STOP_BITS	1

/* Timer IRQ priority - must be lower than IDL_TIM_IRQ_PRIORITY */
#define SLP_TIM_IRQ_PRIORITY	7

/* Timer ID */
#define SLEEP_TIMER		TIMER5

/* GPIO pin connected to an LED to hint an error */
#define ERROR_LED_PIN		PA5

#endif
