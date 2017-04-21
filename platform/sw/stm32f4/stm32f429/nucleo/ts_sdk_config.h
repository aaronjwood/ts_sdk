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

/* UART and timer IRQ priorities - UART must have higher priority than timer */
#define MODEM_UART_IRQ_PRIORITY	5
#define TIMER_IRQ_PRIORITY

#endif
