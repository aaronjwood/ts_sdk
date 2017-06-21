/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * This header file contains the configuration for different module
 * tests for specific boards.
 */

#if defined (beduin)

/* GPIO pin connected to an LED to verify the GPIO test */
#define GPIO_LED_PIN		PC13

/* BME280 sensor registers and values */
#define I2C_SCL                 PB6   /* I2C Serial clock pin */
#define I2C_SDA                 PB7   /* I2C Serical data pin */
#define SLAVE_ADDR              0x77  /* I2C device address */
#define SLAVE_ID                0xD0  /* Device identfication register */
#define SLAVE_IDENTIFIER        0x60  /* Device identification value */
#define CTRL_REG                0xF4  /* Sensor control register */
#define CALIB_SZ                26    /* calibration table length */

/* Other module conifgs */

#elif defined (nucleo)

/* GPIO pin connected to an LED to verify the GPIO test */
#define GPIO_LED_PIN		PB14

/* xnucleo-iks01a1 hts21 sensor registers and values */
#define I2C_SCL                 PB8   /* I2C Serial clock pin */
#define I2C_SDA                 PB9   /* I2C Serial data pin */
#define SLAVE_ADDR              0x5f  /* I2C Device address  */
#define SLAVE_ID                0x0f  /* Device identfication register */
#define SLAVE_IDENTIFIER        0xBC  /* Device identfication value */
#define CTRL_REG                0x20  /* Sensor control register */
#define CALIB_SZ                16    /*calibration table length */

/* Other module conifgs */

#else

#error "define valid board options from beduin or nucleo"

#endif
