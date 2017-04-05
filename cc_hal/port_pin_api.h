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
 * this function. On success the pin is marked as used.
 *
 * \param[in] pin_name Name of the pin
 * \param[in] mapping A pointer to the mapping data structure used by the peripheral
 *
 * \retval true The pin was successfully initialized to work with the peripheral
 * \retval false Failed to initialize the pin. Possible causes are:
 * 	        \arg The pin is already being used
 * 	        \arg The pin is not connected to the peripheral
 */
bool pp_peripheral_pin_init(pin_name_t pin_name, const pin_t *mapping);

#endif
