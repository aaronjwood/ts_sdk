/**
 * \file Configures the board specific pin and peripheral mappings that relate
 * to the SDK.
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \details This file sets which UART pins to use to communicate with the modem,
 * IRQ priorities and any other peripherals needed by the SDK. \n
 *
 * Target board : Beduin \n
 * Target MCU   : STM32F415RGT
 */
#ifndef TS_SDK_BOARD_CONFIG_H
#define TS_SDK_BOARD_CONFIG_H

/* Pin and peripheral configuration for UART connecting modem and MCU */
#define MODEM_UART_TX_PIN	PB10
#define MODEM_UART_RX_PIN	PB11
#define MODEM_UART_RTS_PIN	PB14
#define MODEM_UART_CTS_PIN	PB13
#define MODEM_HW_RESET_PIN	PB3
#define MODEM_UART_BAUD_RATE	115200
#define MODEM_UART_DATA_WIDTH	8
#define MODEM_UART_PARITY	NONE
#define MODEM_UART_STOP_BITS	1

/* Pin and peripheral configuration for UART connecting GPS */
#define GPS_TX_PIN              PC10
#define GPS_RX_PIN              PC11
#define GPS_BAUD_RATE           9600
#define GPS_UART_DATA_WIDTH     8
#define GPS_UART_PARITY         0
#define GPS_UART_STOP_BITS_1    1
#define GPS_UART_IRQ_PRIORITY   0

/* UART and timer IRQ priorities - UART must have higher priority than timer */
#define MODEM_UART_IRQ_PRIORITY	5
#define IDL_TIM_IRQ_PRIORITY	6

/* Timer ID */
#define MODEM_UART_IDLE_TIMER	TIMER2

#endif
