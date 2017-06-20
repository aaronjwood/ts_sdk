/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * This header file contains the configuration for different module
 * tests for specific boards.
 */

#if defined (beduin)

/* GPIO pin connected to an LED to verify the GPIO test */
#define GPIO_LED_PIN		PC13

/* Other module conifgs */

#elif defined (nucleo)

/* GPIO pin connected to an LED to verify the GPIO test */
#define GPIO_LED_PIN		PB14

/* Other module conifgs */

#else

#error "define valid board options from beduin or nucleo"

#endif
