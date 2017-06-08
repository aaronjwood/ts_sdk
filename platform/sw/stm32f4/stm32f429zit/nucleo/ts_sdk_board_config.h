/**
 * \file Configures the board specific pin and peripheral mappings that relate
 * to the SDK.
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \details This file sets which UART pins to use to communicate with the modem,
 * IRQ priorities and any other peripherals needed by the SDK. \n
 *
 * Target board : Nucleo-F429ZI \n
 * Target MCU   : STM32F429ZIT
 */
#ifndef TS_SDK_BOARD_CONFIG_H
#define TS_SDK_BOARD_CONFIG_H

/* Pin and peripheral configuration for UART connecting modem and MCU */
#define MODEM_UART_TX_PIN	PD5
#define MODEM_UART_RX_PIN	PD6
#define MODEM_UART_RTS_PIN	PD4
#define MODEM_UART_CTS_PIN	PD3
#define MODEM_HW_RESET_PIN	PD2
#define MODEM_UART_BAUD_RATE	115200
#define MODEM_UART_DATA_WIDTH	8
#define MODEM_UART_PARITY	NONE
#define MODEM_UART_STOP_BITS	1

/* UART and timer IRQ priorities - UART must have higher priority than timer */
#define MODEM_UART_IRQ_PRIORITY	5
#define IDL_TIM_IRQ_PRIORITY	6

/* Timer ID */
#define MODEM_UART_IDLE_TIMER	TIMER2

/*
 * Uncomment the following to set a custom APN. If they are commented out, the
 * modem will attempt to use an existing APN. Both values must be defined for
 * the APN to be used. APN type and value are modem specific.
 */

/* M2M SIM card APN */
/* #define MODEM_APN_TYPE		"IP" */
/* #define MODEM_APN_VALUE		"UWSEXT.GW15.VZWENTP" */

#endif
