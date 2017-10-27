/*
 * Copyright (C) 2017 Verizon. All rights reserved.
 * Configuration header for U-Blox TOBY-L201 for various protocols and SIM
 * cards.
 */
#ifndef TS_SDK_MODEM_CONFIG_H
#define TS_SDK_MODEM_CONFIG_H

/*
 * Define the SIM type being used. Possible values are:
 * o M2M - Machine-to-Machine
 * o COM - Commercial
 * o DAK_LAB - Dakota Lab SIM (SMSNAS LAB network)
 * o DAK_COM - Dakota Commercial SIM (SMSNAS Commercial network)
 */
#define M2M			0
#define COM			1
#define DAK_LAB			2
#define DAK_COM			3

/*
 * The SIM_TYPE macro can be used to specialize the configuration for a given
 * scenario. Example: With protocol set to SMSNAS, UMNOCONF takes "0,0" when
 * using Dakota SIMs and takes "1,6" when using another type of SIM.
 */
#define SIM_TYPE		COM


/* SIM specific APN settings */
#if SIM_TYPE == M2M

#define MODEM_APN_CTX_ID	"1"
#define MODEM_APN_TYPE		"IP"
#define MODEM_APN_TYPE_ID	"0"
#define MODEM_APN_VALUE		"UWSEXT.GW15.VZWENTP"

#elif SIM_TYPE == COM

#define MODEM_APN_TYPE_ID	"2"
#define MODEM_APN_CTX_ID	"8"

#endif	/* SIM_TYPE */


/* Protocol and SIM specific settings */
#if defined(OTT_PROTOCOL) || defined(MQTT_PROTOCOL)

#define MODEM_UMNOCONF_VAL	"3,23"

#elif defined(SMSNAS_PROTOCOL)

#if SIM_TYPE == DAK_LAB
#define MODEM_UMNOCONF_VAL	"1,7"
#elif SIM_TYPE == DAK_COM
#define MODEM_UMNOCONF_VAL	"0,7"
#else
#define MODEM_UMNOCONF_VAL	"1,6"
#endif	/* SIM_TYPE */

#elif defined(NO_PROTOCOL)
#define MODEM_UMNOCONF_VAL	"3,23"

#else

#error "Must specify a protocol"

#endif	/* PROTOCOL */


/* Modem communications port settings */
#if defined(stm32f429zit)
#if defined(nucleo)
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
#endif	/* BOARD */

#elif defined(stm32l476rgt)
#if defined(nucleo)
#define MODEM_UART_TX_PIN	PA9
#define MODEM_UART_RX_PIN	PA10
#define MODEM_UART_RTS_PIN	PA12
#define MODEM_UART_CTS_PIN	PA11
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
#endif	/* BOARD */

#elif defined(stm32f415rgt)
#if defined(beduin)
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

/* UART and timer IRQ priorities - UART must have higher priority than timer */
#define MODEM_UART_IRQ_PRIORITY	5
#define IDL_TIM_IRQ_PRIORITY	6

/* Timer ID */
#define MODEM_UART_IDLE_TIMER	TIMER2
#endif	/* BOARD */

#else
#error "Must specify chipset and board"

#endif /* MCU */
#endif /* TS_SDK_MODEM_CONFIG_H */
