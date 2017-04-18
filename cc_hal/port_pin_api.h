/**
 * \file port_pin_api.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Abstraction over the ports and pins of the MCU.
 * \details This API allows to record & query the usage of pins on the MCU and
 * initialize the pins of a peripheral.
 */
#ifndef PP_API_H
#define PP_API_H

#include <stdbool.h>
#include "pin_map.h"

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
 * \brief Find the peripheral the pin is connected to.
 * \details Use this routine to verify if two different pins are connected to
 * the same peripheral. If they are, they can be initialized to be interfaces
 * of that peripheral.
 *
 * \param[in] pin_name Name of the pin
 * \param[in] mapping A pointer to the mapping data structure used by the peripheral
 *
 * \returns A handle to the peripheral this pin is connected to. If no such
 * peripheral is found, \ref NO_PERIPH is returned.
 */
periph_t pp_get_peripheral(pin_name_t pin_name, const pin_map_t *mapping);

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
bool pp_peripheral_pin_init(pin_name_t pin_name, const pin_map_t *mapping);

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

/**
 * \brief Get the underlying driver's representation of the pin number.
 * \param[in] pin_name Name of the pin
 * \returns Representation of the pin compatible with the underlying driver.
 */
uint32_t pp_map_drv_pin(pin_name_t pin_name);

/**
 * \brief Get the underlying driver's representation of the port.
 * \param[in] pin_name Name of the pin
 * \returns Representation of the port compatible with the underlying driver.
 */
uint32_t pp_map_drv_port(pin_name_t pin_name);

#endif
