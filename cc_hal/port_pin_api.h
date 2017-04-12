/**
 * \file port_pin_api.h
 * \brief Abstraction over the ports and pins of the MCU.
 * \details This API allows to record & query the usage of pins on the MCU and
 * initialize the pins of a peripheral.
 */
#ifndef PP_API_H
#define PP_API_H

#include <stdbool.h>
#include "pin_map.h"

#define NOT_FOUND	((uint32_t)0xFFFFFFFF) /**< Stands for a value not found*/

/**
 * \brief Mark the pin as used.
 * \details If the pin is already marked as used, this function does nothing.
 *
 * \param[in] pin_name Name of the pin
 */
void pp_mark_pin_used(pin_name_t pin_name);

/**
 * \brief Query if the pin is being used.
 *
 * \param[in] pin_name Name of the pin
 *
 * \retval true Pin is already mapped
 * \retval false Pin is free to be used
 */
bool pp_is_pin_used(pin_name_t pin_name);

/**
 * \brief Initialize the pin to function as an interface to a peripheral.
 * \details Pin settings are derived from a mapping data structure passed to
 * this function. On success, the pin is marked as used.
 *
 * \param[in] pin_name Name of the pin
 * \param[in] mapping A pointer to the mapping data structure used by the peripheral
 *
 * \retval true The pin was successfully initialized to work with the peripheral
 * \retval false Failed to initialize the pin. Possible causes are:
 * 	        \arg Pin is already being used
 * 	        \arg Pin is not connected to the peripheral
 * 	        \arg Pin name is invalid
 */
bool pp_peripheral_pin_init(pin_name_t pin_name, const pin_t *mapping);

/**
 * \brief Initialize the pin to function as a GPIO.
 * \details Pin settings are derived from the settings parameter passed to
 * this function. On success, the pin is marked as used.
 *
 * \param[in] pin_name Name of the pin
 * \param[in] settings A pointer to a \ref gpio_config_t that holds the GPIO
 * settings
 *
 * \retval true Successfully initialized the pin to work as a GPIO
 * \retval false Failed to initialized the pin. Possible causes are:
 * 		\arg Pin is already being used
 * 		\arg Pin cannot be used as GPIO
 * 		\arg Pin name is invalid
 */
bool pp_gpio_pin_init(pin_name_t pin_name, const gpio_config_t *settings);

#endif
