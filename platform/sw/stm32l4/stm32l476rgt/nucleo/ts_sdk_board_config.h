/**
 * \file Configures the board specific pin and peripheral mappings that relate
 * to the SDK.
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \details This file sets which UART pins to use to communicate with the modem,
 * IRQ priorities and any other peripherals needed by the SDK. \n
 *
 * Target board : Nucleo-L476RG \n
 * Target MCU   : STM32L476RGT
 */
#ifndef TS_SDK_BOARD_CONFIG_H
#define TS_SDK_BOARD_CONFIG_H

/* Pin and peripheral configuration for UART connecting modem and MCU */
#define MODEM_UART_TX_PIN	PC10
#define MODEM_UART_RX_PIN	PC11
#define MODEM_UART_RTS_PIN	PA15
#define MODEM_UART_CTS_PIN	PB7
#define MODEM_HW_RESET_PIN	PB0
#define MODEM_UART_BAUD_RATE	115200
#define MODEM_UART_DATA_WIDTH	8
#define MODEM_UART_PARITY	NONE
#define MODEM_UART_STOP_BITS	1

/* UART and timer IRQ priorities - UART must have higher priority than timer */
#define MODEM_UART_IRQ_PRIORITY	5
#define IDL_TIM_IRQ_PRIORITY	6

/* Timer ID */
#define MODEM_UART_IDLE_TIMER	TIMER2

#endif
