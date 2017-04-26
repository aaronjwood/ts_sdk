#ifndef TS_SDK_CONFIG_H
#define TS_SDK_CONFIG_H

/* Pin and peripheral configuration for UART connecting modem and MCU */
#define MODEM_UART_TX_PIN	PD5
#define MODEM_UART_RX_PIN	PD6
#define MODEM_UART_RTS_PIN	PD4
#define MODEM_UART_CTS_PIN	PD3
#define MODEM_UART_BAUD_RATE	115200
#define MODEM_UART_DATA_WIDTH	8
#define MODEM_UART_PARITY	NONE
#define MODEM_UART_STOP_BITS	1

/* Pin and peripheral configuration for the debug UART */
#define DEBUG_UART_TX_PIN	PC10
#define DEBUG_UART_RX_PIN	NC
#define DEBUG_UART_RTS_PIN	NC
#define DEBUG_UART_CTS_PIN	NC
#define DEBUG_UART_BAUD_RATE	115200
#define DEBUG_UART_DATA_WIDTH	8
#define DEBUG_UART_PARITY	NONE
#define DEBUG_UART_STOP_BITS	1

/* UART and timer IRQ priorities - UART must have higher priority than timer */
#define MODEM_UART_IRQ_PRIORITY	5
#define IDL_TIM_IRQ_PRIORITY	6

/* Timer IDs */
#define MODEM_UART_IDLE_TIMER	TIMER2

#endif
