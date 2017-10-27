/**
 * \file pin_defs.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Defines pins and ports supported by a specific chipset.
 * \details Variant : \b Raspberry Pi 3 \n
 */
#ifndef PIN_DEFS_H
#define PIN_DEFS_H

#include <stdint.h>
#include <stdbool.h>

#include "pin_std_defs.h"

typedef enum {
	NUM_PORTS
} port_id_t;

typedef uint8_t pin_id_t;

/**
 * \brief Type that represents the alternate functions of the GPIO pins.
 */
enum alt_func {
	AF_NONE
};

enum pin_name {
	FORCE_WIDTH_32BITS = 0xFFFFFFFEu
};

/**
 * \brief Peripheral pin map element.
 */
struct pin_map {
	enum pin_name pin_name;	/**< Associated pin name from the above list */
	gpio_config_t settings;	/**< GPIO pin configuration to go with peripheral */
	enum alt_func alt_func;	/**< Alternate function ID for the pin */
	periph_t peripheral;	/**< Associated peripheral, if any */
};

#endif
