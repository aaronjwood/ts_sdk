/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * This header file contains the gps configuration for the 
 * for specific boards.
 * Currently GPS_CHIPSET=neo-6m is supported only for 
 * CHIPSET_MCU = stm32f415rgt
 */

#if defined(beduin) || defined(nomad)
/* Pin and peripheral configuration for UART connecting GPS */
#define GPS_TX_PIN              PC10
#define GPS_RX_PIN              PC11
#define GPS_BAUD_RATE           9600
#define GPS_UART_DATA_WIDTH     8
#define GPS_UART_PARITY         0
#define GPS_UART_STOP_BITS_1    1
#define GPS_UART_IRQ_PRIORITY   0

#else

#error "define valid board options from beduin or nucleo"

#endif
