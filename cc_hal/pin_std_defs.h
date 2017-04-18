/**
 * \file pin_std_defs.h
 * \brief Provides standard definitions for the HAL.
 * \details \b Porting \b Guide: \n
 * This header provides the minimum abstractions needed to port the rest
 * of the HAL to other platforms. Each platform must define the enumeration
 * pin_name and the structure %pin_map.
 * The enumeration defines the pin names (refer \ref pin_name_t) available
 * on this target while the structure describes the element of a peripheral map
 * (refer \ref pin_map_t).In addition to these two elements,, a generic type
 * (\ref gpio_config_t) representing a GPIO configuration is provided by this
 * header.
 */
#ifndef PIN_STD_DEFS_H
#define PIN_STD_DEFS_H

/**
 * \brief Stands for the absence of a peripheral.
 */
#define NO_PERIPH	((periph_t)(0xFFFFFFFFu))

/**
 * \brief Stands for the absence of a pin.
 */
#define NC		((uint32_t)(0xFFFFFFFFu))

/**
 * \brief Type that represents the pin name.
 * \details Pins are named according to the following convention: \n
 * P\<port letter\>\<pin ID\>. \n
 * For example, Port A Pin 4 is named PA4. Each member of this enumeration must
 * encode its corresponding port and pin within its value. Absence of a pin is
 * represented by \ref NC.
 */
typedef enum pin_name pin_name_t;

/**
 * \brief Type that stores a single peripheral pin configuration.
 * \details A peripheral pin configuration has enough information to configure
 * the pin to become the interface of a given peripheral. Each entry in the
 * peripheral pin map is of this type.
 */
typedef struct pin_map pin_map_t;

/**
 * \brief Type wide enough to hold a handle to a peripheral.
 * \details Absence of a peripheral is represented by \ref NO_PERIPH.
 */
typedef uint32_t periph_t;

/**
 * \brief Type that defines the direction of the GPIO.
 */
typedef enum dir {
	INPUT,		/**< GPIO pin is set as input */
	OUTPUT		/**< GPIO pin is set as output */
} dir_t;

/**
 * \brief Type that defines the output type and pull configuration of the pin.
 */
typedef enum pull_mode {
	OD_NO_PULL,	/**< Open Drain output without any pull */
	OD_PULL_UP,	/**< Open Drain output with a weak pull up */
	OD_PULL_DOWN,	/**< Open Drain output with a weak pull down */
	PP_NO_PULL,	/**< Push Pull output without any pull */
	PP_PULL_UP,	/**< Push Pull output with a weak pull up */
	PP_PULL_DOWN	/**< Push Pull output with a weak pull down */
} pull_mode_t;

/**
 * \brief Type that defines the output pin speed.
 * \details This determines the electrical characteristics of the pin.
 */
typedef enum speed {
	SPEED_LOW,	/**< Low Speed pin */
	SPEED_MEDIUM,	/**< Medium Speed pin */
	SPEED_HIGH,	/**< High Speed pin */
	SPEED_VERY_HIGH	/**< Very high speed pin */
} speed_t;

/**
 * \brief Type that represents a GPIO configuration.
 */
typedef struct {
	/** Direction of the GPIO pin. Accepts \ref dir_t. */
	uint8_t dir : 1;

	/** Output type and pull configuration. Accepts \ref pull_mode_t. */
	uint8_t pull_mode : 3;

	/** Maximum Speed of the pin. Accepts \ref speed_t. */
	uint8_t speed : 2;

} __attribute__((packed)) gpio_config_t;

#endif
