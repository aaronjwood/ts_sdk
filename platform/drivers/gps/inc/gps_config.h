/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * This header file contains the configuration for different module
 * tests for specific boards.
 */

#if defined(beduin)
/* Pin and peripheral configuration for UART connecting GPS */
#define GPS_TX_PIN              PC10
#define GPS_RX_PIN              PC11
#define GPS_BAUD_RATE           9600
#define GPS_UART_DATA_WIDTH     8
#define GPS_UART_PARITY         0
#define GPS_UART_STOP_BITS_1    1
#define GPS_UART_IRQ_PRIORITY   0

/* Other module configs */
#elif defined(nucleo)

/* xnucleo-iks01a1 gps sensor registers and values */
#if defined(stm32l476rgt)
#define GPS_I2C_SCL	PB8   /* GPS I2C Serial clock pin */
#define GPS_I2C_SDA	PB9   /* GPS I2C Serial data pin */
#define GPS_SLAVE_ADDR	0x24  /* GPS I2C device address */
#define GPS_RESET_PIN	PB5
#define GPS_DOR_PIN	PC1
#endif

#else

#error "define valid board options from beduin or nucleo"

#endif
