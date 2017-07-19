/*
 * Copyright (C) 2017 Verizon. All rights reserved.
 * Configuration header for Sequans Monarch CAT-M modem for various protocols and SIM cards.
 */
#ifndef TS_SDK_MODEM_CONFIG_H
#define TS_SDK_MODEM_CONFIG_H

#if defined (stm32f429zit)	/* XXX: Delete later */
#ifdef nucleo
/* Pin and peripheral configuration for UART connecting modem and MCU */
#define MODEM_UART_TX_PIN	PD5
#define MODEM_UART_RX_PIN	PD6
#define MODEM_UART_RTS_PIN	NC
#define MODEM_UART_CTS_PIN	NC
#define MODEM_HW_RESET_PIN	PG3
#define MODEM_HW_PWREN_PIN	PG2
#define MODEM_UART_BAUD_RATE	115200
#define MODEM_UART_DATA_WIDTH	8
#define MODEM_UART_PARITY	NONE
#define MODEM_UART_STOP_BITS	1

#define MODEM_EMULATED_RTS	PD3
#define MODEM_EMULATED_CTS	PD4

/* UART and timer IRQ priorities - UART must have higher priority than timer */
#define MODEM_UART_IRQ_PRIORITY	5
#define IDL_TIM_IRQ_PRIORITY	6

/* Timer ID */
#define MODEM_UART_IDLE_TIMER	TIMER2

#define MODEM_PDP_CTX		"3"
#define MODEM_SOCK_ID		"1"

#endif	/* BOARD */

#elif defined (stm32l476rgt)
#ifdef nucleo
/* Pin and peripheral configuration for UART connecting modem and MCU */
#define MODEM_UART_TX_PIN	PD5
#define MODEM_UART_RX_PIN	PD6
#define MODEM_UART_RTS_PIN	NC
#define MODEM_UART_CTS_PIN	NC
#define MODEM_HW_RESET_PIN	PG3
#define MODEM_HW_PWREN_PIN	PG2
#define MODEM_UART_BAUD_RATE	115200
#define MODEM_UART_DATA_WIDTH	8
#define MODEM_UART_PARITY	NONE
#define MODEM_UART_STOP_BITS	1

#define MODEM_EMULATED_RTS	PD3
#define MODEM_EMULATED_CTS	PD4

/* UART and timer IRQ priorities - UART must have higher priority than timer */
#define MODEM_UART_IRQ_PRIORITY	5
#define IDL_TIM_IRQ_PRIORITY	6

/* Timer ID */
#define MODEM_UART_IDLE_TIMER	TIMER2

#define MODEM_PDP_CTX		"3"
#define MODEM_SOCK_ID		"1"

#endif	/* BOARD */

#else
#error "Must specify chipset and board"
#endif	/* MCU */
#endif	/* TS_SDK_MODEM_CONFIG */
