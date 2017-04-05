/**
 * \file pin_defs.h
 * \brief Defines pins and ports supported by the chipset.
 * \details Variant : \b STM32F4xx \n \n
 * \b Porting \b Guide: \n
 * This header forms the base for all peripheral related HALs that rely on the
 * pins of the MCU. It must expose the following symbols:
 * 	- A \ref port_id_t type that represents a GPIO port
 * 	- A \ref pin_id_t type that represents a pin number of a GPIO port
 * 	- A \ref pin_name_t type that represents a pin name (Eg. PA4, PC0 etc.)
 * 	- A \ref pin_t that encapsulates all information needed to configure a
 * 	pin for a given peripheral
 * 	- A \ref dir_t type that defines the directions the GPIO pin can take
 * 	- A \ref pull_mode_t type that represents the output type and pull up
 * 	resistor values
 * 	- A \ref gpio_config_t that encapsulates all information needed to
 * 	configure a pin as a GPIO not attached to a peripheral
 * 	- A \ref speed_t type that defines the maximum speed of the pin
 * 	- A \ref pd_get_port routine to retrieve the \ref port_id_t from a
 * 	\ref pin_name_t
 * 	- A \ref pd_get_pin routine to retrieve the \ref pin_id_t from a
 * 	\ref pin_name_t
 */
#ifndef PIN_DEF_H
#define PIN_DEF_H

#include <stdint.h>

#ifndef IN_DOXYGEN
#define ATTRIB_PACKED	__attribute__((packed))
#else
#define ATTRIB_PACKED
#endif

/**
 * \brief Type that represents the ports available for this chipset.
 */
typedef enum port_id {
	PORT_A,		/**< PORT A */
	PORT_B,		/**< PORT B */
	PORT_C,		/**< PORT C */
	PORT_D,		/**< PORT D */
	PORT_E,		/**< PORT E */
	PORT_F,		/**< PORT F */
	PORT_G,		/**< PORT G */
	PORT_H,		/**< PORT H */
	NUM_PORTS	/**< Maximum number of ports */
} port_id_t;

/**
 * \brief Type that stores the pin number of a port.
 */
typedef uint8_t pin_id_t;

#define PD_GET_NAME(port, pin)	((uint8_t)(port << 4 | pin))

/**
 * \brief Type that represents the pin name.
 * \details Pins are named according to the following convention: \n
 * P\<port letter\>\<pin ID\>. \n
 * For example, Port A Pin 4 is named PA4.
 */
typedef enum pin_name {
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

	PD0 = PD_GET_NAME(PORT_D, 0),
	PD1 = PD_GET_NAME(PORT_D, 1),
	PD2 = PD_GET_NAME(PORT_D, 2),
	PD3 = PD_GET_NAME(PORT_D, 3),
	PD4 = PD_GET_NAME(PORT_D, 4),
	PD5 = PD_GET_NAME(PORT_D, 5),
	PD6 = PD_GET_NAME(PORT_D, 6),
	PD7 = PD_GET_NAME(PORT_D, 7),
	PD8 = PD_GET_NAME(PORT_D, 8),
	PD9 = PD_GET_NAME(PORT_D, 9),
	PD10 = PD_GET_NAME(PORT_D, 10),
	PD11 = PD_GET_NAME(PORT_D, 11),
	PD12 = PD_GET_NAME(PORT_D, 12),
	PD13 = PD_GET_NAME(PORT_D, 13),
	PD14 = PD_GET_NAME(PORT_D, 14),
	PD15 = PD_GET_NAME(PORT_D, 15),

	PE0 = PD_GET_NAME(PORT_E, 0),
	PE1 = PD_GET_NAME(PORT_E, 1),
	PE2 = PD_GET_NAME(PORT_E, 2),
	PE3 = PD_GET_NAME(PORT_E, 3),
	PE4 = PD_GET_NAME(PORT_E, 4),
	PE5 = PD_GET_NAME(PORT_E, 5),
	PE6 = PD_GET_NAME(PORT_E, 6),
	PE7 = PD_GET_NAME(PORT_E, 7),
	PE8 = PD_GET_NAME(PORT_E, 8),
	PE9 = PD_GET_NAME(PORT_E, 9),
	PE10 = PD_GET_NAME(PORT_E, 10),
	PE11 = PD_GET_NAME(PORT_E, 11),
	PE12 = PD_GET_NAME(PORT_E, 12),
	PE13 = PD_GET_NAME(PORT_E, 13),
	PE14 = PD_GET_NAME(PORT_E, 14),
	PE15 = PD_GET_NAME(PORT_E, 15),

	PF0 = PD_GET_NAME(PORT_F, 0),
	PF1 = PD_GET_NAME(PORT_F, 1),
	PF2 = PD_GET_NAME(PORT_F, 2),
	PF3 = PD_GET_NAME(PORT_F, 3),
	PF4 = PD_GET_NAME(PORT_F, 4),
	PF5 = PD_GET_NAME(PORT_F, 5),
	PF6 = PD_GET_NAME(PORT_F, 6),
	PF7 = PD_GET_NAME(PORT_F, 7),
	PF8 = PD_GET_NAME(PORT_F, 8),
	PF9 = PD_GET_NAME(PORT_F, 9),
	PF10 = PD_GET_NAME(PORT_F, 10),
	PF11 = PD_GET_NAME(PORT_F, 11),
	PF12 = PD_GET_NAME(PORT_F, 12),
	PF13 = PD_GET_NAME(PORT_F, 13),
	PF14 = PD_GET_NAME(PORT_F, 14),
	PF15 = PD_GET_NAME(PORT_F, 15),

	PG0 = PD_GET_NAME(PORT_G, 0),
	PG1 = PD_GET_NAME(PORT_G, 1),
	PG2 = PD_GET_NAME(PORT_G, 2),
	PG3 = PD_GET_NAME(PORT_G, 3),
	PG4 = PD_GET_NAME(PORT_G, 4),
	PG5 = PD_GET_NAME(PORT_G, 5),
	PG6 = PD_GET_NAME(PORT_G, 6),
	PG7 = PD_GET_NAME(PORT_G, 7),
	PG8 = PD_GET_NAME(PORT_G, 8),
	PG9 = PD_GET_NAME(PORT_G, 9),
	PG10 = PD_GET_NAME(PORT_G, 10),
	PG11 = PD_GET_NAME(PORT_G, 11),
	PG12 = PD_GET_NAME(PORT_G, 12),
	PG13 = PD_GET_NAME(PORT_G, 13),
	PG14 = PD_GET_NAME(PORT_G, 14),
	PG15 = PD_GET_NAME(PORT_G, 15),

	PH0 = PD_GET_NAME(PORT_H, 0),
	PH1 = PD_GET_NAME(PORT_H, 1),

	NC = 0xFFFFFFFF		/**< Used to denote a pin that's not connected */
} pin_name_t;

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
	OD_NO_PULLL,	/**< Open Drain output without any pull */
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
 * \brief Type that represents the alternate functions of the GPIO pins.
 */
typedef enum alt_func {
	AF0,	/**< Alternate function 0 */
	AF1,	/**< Alternate function 1 */
	AF2,	/**< Alternate function 2 */
	AF3,	/**< Alternate function 3 */
	AF4,	/**< Alternate function 4 */
	AF5,	/**< Alternate function 5 */
	AF6,	/**< Alternate function 6 */
	AF7,	/**< Alternate function 7 */
	AF8,	/**< Alternate function 8 */
	AF9,	/**< Alternate function 9 */
	AF10,	/**< Alternate function 10 */
	AF11,	/**< Alternate function 11 */
	AF12,	/**< Alternate function 12 */
	AF13,	/**< Alternate function 13 */
	AF14,	/**< Alternate function 14 */
	AF15	/**< Alternate function 15 */
} alt_func_t;

/**
 * \brief Type that represents a GPIO configuration.
 */
typedef struct {
	uint8_t dir : 1;	/**< Direction of the GPIO pin.
				  Accepts \ref dir_t. */
	uint8_t pull_mode : 3;	/**< Output type and pull configuration.
				  Accepts \ref pull_mode_t. */
	uint8_t speed : 2;	/**< Maximum Speed of the pin.
				  Accepts \ref speed_t. */
} gpio_config_t;


/**
 * \brief Structure that stores a single pin configuration.
 * \details A pin configuration has enough information to configure the pin to
 * become the interface of a given peripheral. Each entry in the peripheral pin
 * map is of this type. \ref END_OF_MAP is a special instance of this structure
 * marking the end of the peripheral map.
 */
typedef struct {
	pin_name_t pin_name;	/**< Associated pin name from the above list */
	gpio_config_t settings;	/**< GPIO pin configuration to go with peripheral */
	uint8_t alt_func;	/**< Alternate function ID */
	uint32_t peripheral;	/**< Associated peripheral, if any */
} pin_t;

/**
 * \brief Marks the end of the peripheral pin map.
 */
#define END_OF_MAP	{NC, {0}, 0, 0}

/**
 * \brief Retrieve the port associated with the pin name
 * \param[in] pin_name Name of the pin
 * \return Port ID of the pin
 */
static inline port_id_t pd_get_port(pin_name_t pin_name)
{
	return ((port_id_t)(pin_name >> 4));
}

/**
 * \brief Retrieve the pin associated with the pin name
 * \param[in] pin_name Name of the pin
 * \return Pin ID associated with the pin name
 */
static inline pin_id_t pd_get_pin(pin_name_t pin_name)
{
	return ((pin_id_t)(pin_name & 0x0F));
}

#endif
