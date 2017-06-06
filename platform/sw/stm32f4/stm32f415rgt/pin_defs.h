/**
 * \file pin_defs.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Defines pins and ports supported by a specific chipset.
 * \details Variant : \b STM32F429RGT \n
 */
#ifndef PIN_DEFS_H
#define PIN_DEFS_H

#include <stdint.h>
#include <stdbool.h>

#include "pin_std_defs.h"

typedef enum {
	PORT_A,
	PORT_B,
	PORT_C,
	PORT_D,
	PORT_H,
	NUM_PORTS
} port_id_t;

typedef uint8_t pin_id_t;

#define PD_GET_NAME(port, pin)	((uint8_t)(port << 4 | pin))
#define PD_GET_PORT(pin_name)	((port_id_t)(pin_name >> 4))
#define PD_GET_PIN(pin_name)	((port_id_t)(pin_name & 0x0F))
#define NUM_PINS_PORT_A_TO_C		16
#define NUM_PINS_PORT_D			1
#define NUM_PINS_PORT_H			2

/**
 * \brief Type that represents the alternate functions of the GPIO pins.
 */
enum alt_func {
	AF0,
	AF1,
	AF2,
	AF3,
	AF4,
	AF5,
	AF6,
	AF7,
	AF8,
	AF9,
	AF10,
	AF11,
	AF12,
	AF13,
	AF14,
	AF15,
	AF_NONE
};

enum pin_name {
	PA0 = PD_GET_NAME(PORT_A, 0),
	PA1 = PD_GET_NAME(PORT_A, 1),
	PA2 = PD_GET_NAME(PORT_A, 2),
	PA3 = PD_GET_NAME(PORT_A, 3),
	PA4 = PD_GET_NAME(PORT_A, 4),
	PA5 = PD_GET_NAME(PORT_A, 5),
	PA6 = PD_GET_NAME(PORT_A, 6),
	PA7 = PD_GET_NAME(PORT_A, 7),
	PA8 = PD_GET_NAME(PORT_A, 8),
	PA9 = PD_GET_NAME(PORT_A, 9),
	PA10 = PD_GET_NAME(PORT_A, 10),
	PA11 = PD_GET_NAME(PORT_A, 11),
	PA12 = PD_GET_NAME(PORT_A, 12),
	PA13 = PD_GET_NAME(PORT_A, 13),
	PA14 = PD_GET_NAME(PORT_A, 14),
	PA15 = PD_GET_NAME(PORT_A, 15),

	PB0 = PD_GET_NAME(PORT_B, 0),
	PB1 = PD_GET_NAME(PORT_B, 1),
	PB2 = PD_GET_NAME(PORT_B, 2),
	PB3 = PD_GET_NAME(PORT_B, 3),
	PB4 = PD_GET_NAME(PORT_B, 4),
	PB5 = PD_GET_NAME(PORT_B, 5),
	PB6 = PD_GET_NAME(PORT_B, 6),
	PB7 = PD_GET_NAME(PORT_B, 7),
	PB8 = PD_GET_NAME(PORT_B, 8),
	PB9 = PD_GET_NAME(PORT_B, 9),
	PB10 = PD_GET_NAME(PORT_B, 10),
	PB11 = PD_GET_NAME(PORT_B, 11),
	PB12 = PD_GET_NAME(PORT_B, 12),
	PB13 = PD_GET_NAME(PORT_B, 13),
	PB14 = PD_GET_NAME(PORT_B, 14),
	PB15 = PD_GET_NAME(PORT_B, 15),

	PC0 = PD_GET_NAME(PORT_C, 0),
	PC1 = PD_GET_NAME(PORT_C, 1),
	PC2 = PD_GET_NAME(PORT_C, 2),
	PC3 = PD_GET_NAME(PORT_C, 3),
	PC4 = PD_GET_NAME(PORT_C, 4),
	PC5 = PD_GET_NAME(PORT_C, 5),
	PC6 = PD_GET_NAME(PORT_C, 6),
	PC7 = PD_GET_NAME(PORT_C, 7),
	PC8 = PD_GET_NAME(PORT_C, 8),
	PC9 = PD_GET_NAME(PORT_C, 9),
	PC10 = PD_GET_NAME(PORT_C, 10),
	PC11 = PD_GET_NAME(PORT_C, 11),
	PC12 = PD_GET_NAME(PORT_C, 12),
	PC13 = PD_GET_NAME(PORT_C, 13),
	PC14 = PD_GET_NAME(PORT_C, 14),
	PC15 = PD_GET_NAME(PORT_C, 15),

	PD2 = PD_GET_NAME(PORT_D, 2),

	PH0 = PD_GET_NAME(PORT_H, 0),
	PH1 = PD_GET_NAME(PORT_H, 1),

	FORCE_WIDTH_32BITS = 0xFFFFFFFEu
};

/**
 * \brief Peripheral pin map element for the STM32F4xx series.
 */
struct pin_map {
	enum pin_name pin_name;	/**< Associated pin name from the above list */
	gpio_config_t settings;	/**< GPIO pin configuration to go with peripheral */
	enum alt_func alt_func;	/**< Alternate function ID for the pin */
	periph_t peripheral;	/**< Associated peripheral, if any */
};

#endif
