#ifndef GPIO_HAL_H
#define GPIO_HAL_H

#include "pp_api.h"

/* XXX: Not all configurations will be supported across targets */
typedef enum {
	INPUT,
	OUTPUT
} dir_t;

typedef enum {
	OD_NO_PULLL,
	OD_PULL_UP,
	OD_PULL_DOWN,
	PP_NO_PULL,
	PP_PULL_UP,
	PP_PULL_DOWN
} pull_mode_t;

typedef enum {
	SPEED_LOW,
	SPEED_MEDIUM,
	SPEED_HIGH,
	SPEED_VERY_HIGH
} speed_t;

typedef enum {
	PIN_HIGH,
	PIN_LOW
} bit_value_t;

/*
 * Configure the GPIO pin. This function must be called on the given pin, prior
 * to calling any other GPIO HAL routines. Failing to do so will result in
 * undefined behavior.
 *
 * Parameters:
 * 	pin_name - Name of the pin
 * 	dir      - Direction of the pin
 * 	mode     - Pull configuration of the pin
 *	speed    - Expected speed (puts conditions on electrical characteristics)
 *
 * Returns:
 * 	True  - Pin was initialized successfully
 * 	False - Failed to initialize pin as GPIO
 */
bool gpio_init(pin_name_t pin_name, dir_t dir, pull_mode_t mode, speed_t speed);

/*
 * Read the value of the GPIO pin.
 *
 * Parameters:
 * 	pin_name - Name of the pin to be read
 *
 * Returns:
 * 	PIN_HIGH - The pin is held at logical HIGH
 * 	PIN_LOW  - The pin is held at logical LOW
 */
bit_value_t gpio_read(pin_name_t pin_name);

/*
 * Write a value to the GPIO pin.
 *
 * Parameters:
 * 	pin_name - Name of the pin to be written to
 * 	bit      - Bit value that needs to be written to the GPIO
 *
 * Returns:
 * 	None
 */
void gpio_write(pin_name_t pin_name, bit_value_t bit);

/*
 * Turn on or turn off the power to the GPIO submodule.
 *
 * Parameters:
 * 	value - Set to "true" to turn on the power to the submodule. "false"
 * 	        turns off the power to the submodule.
 *
 * Returns:
 * 	None
 */
void gpio_pwr(bool on);

#endif
