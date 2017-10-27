/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * This header file contains the gps configuration for the 
 * for specific boards.
 * Currently GPS_CHIPSET=cxd5600 is supported only for 
 * CHIPSET_MCU = stm32l476rgt 
 */

/* xnucleo-iks01a1 gps sensor registers and values */
#if defined(stm32l476rgt)
#define GPS_I2C_SCL	PB8   /* GPS I2C Serial clock pin */
#define GPS_I2C_SDA	PB9   /* GPS I2C Serial data pin */
#define GPS_SLAVE_ADDR	0x24  /* GPS I2C device address */
#define GPS_RESET_PIN	PB5
#define GPS_DOR_PIN	PC1

#else

#error "define valid board options from beduin or nucleo"

#endif
