#ifndef PP_API_H
#define PP_API_H

/*
 * Pin and port API. This API forms an abstraction over the hardware pins. It
 * provides facilities to ensure pins aren't overused.
 */

#include <stdbool.h>
#include "pin_map.h"

/*
 * Mark the pin as used. If the pin is already marked as used, this function
 * does nothing.
 *
 * Parameters:
 * 	pin_name - Name of the pin
 *
 * Returns:
 * 	None
 */
void pp_mark_pin_used(pin_name_t pin_name);

/*
 * Query if the pin is being used.
 *
 * Parameters:
 * 	pin_name - Name of the pin
 *
 * Returns:
 * 	True  - The pin is already mapped
 * 	False - The pin is free to be used
 */
bool pp_is_pin_used(pin_name_t pin_name);

/*
 * Initialize the pin to function as an interface to a peripheral. Pin settings
 * are derived from a mapping data structure passed to this function. On success
 * the pin is marked as used.
 *
 * Parameters:
 * 	pin_name - Name of the pin
 * 	mapping  - A pointer to the mapping data structure used by the peripheral
 *
 * Returns:
 * 	True  - The pin was successfully initialized to work with the peripheral
 * 	False - Failed to initialize the pin. Possible causes are:
 * 	        a) The pin is already being used
 * 	        b) The pin is not connected to the peripheral
 */
bool pp_peripheral_pin_init(pin_name_t pin_name, const pin_t *mapping);

#endif
