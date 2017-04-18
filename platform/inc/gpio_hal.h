/**
 * \file gpio_hal.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Hardware abstraction layer for GPIO pins
 * \details This header defines a platform independent API to read and write
 * GPIO pins. It also offers a utility routine to control the power to the GPIO
 * submodule. GPIO interrupts are not handled as a part of this API.
 */
#ifndef GPIO_HAL_H
#define GPIO_HAL_H

#include "port_pin_api.h"

/**
 * \brief Type that represents a bit value.
 */
typedef enum bit_value {
	PIN_HIGH,	/**< Logical HIGH */
	PIN_LOW		/**< Logical LOW */
} bit_value_t;

/**
 * \brief Configure the pin to act as GPIO.
 * \details This function must be called on the given pin prior to calling any
 * other GPIO HAL routines. Failing to do so may result in undefined behavior.
 *
 * \param[in] config Pointer to a \ref gpio_config_t data structure describing
 * the pin configuration
 * \param[in] pin_name Name of the pin
 *
 * \retval true Pin was initialized successfully
 * \retval false Failed to initialize the pin as GPIO
 */
bool gpio_init(pin_name_t pin_name, const gpio_config_t *config);

/**
 * \brief Read the value of the GPIO pin.
 * \param[in] pin_name Name of the pin to be read
 * \retval PIN_HIGH The pin held at logical HIGH
 * \retval PIN_LOW The pin is held at logical LOW
 * \pre \ref gpio_init must be called on this pin before reading from it.
 */
bit_value_t gpio_read(pin_name_t pin_name);

/**
 * \brief Write a value to the GPIO pin.
 * \param[in] pin_name Name of the pin to be written to
 * \param[in] bit Bit value that needs to be written to the GPIO
 * \pre \ref gpio_init must be called on this pin before writing to it.
 */
void gpio_write(pin_name_t pin_name, bit_value_t bit);

/**
 * \brief Turn on or off the power to the GPIO port associated with the pin.
 * \param[in] pin_name Name of the pin
 * \param[in] state \arg Set to \b true to turn on the power
 *                  \arg Set to \b false to turn off the power
 * \pre \ref gpio_init must be called on this pin before reading from it.
 */
void gpio_pwr(pin_name_t pin_name, bool state);

#endif
