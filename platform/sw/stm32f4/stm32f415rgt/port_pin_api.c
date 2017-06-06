/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>
#include "port_pin_api.h"

/* Chipset specific implementation. Target: STM32F415RGT */
#include <stm32f4xx_hal.h>

static uint16_t port_usage[NUM_PORTS];

#define MARK_AS_USED(port, pin)		(port_usage[(port)] |= (1 << (pin)))
#define QUERY_USAGE(port, pin)		((port_usage[(port)] & (1 << (pin))) == (1 << (pin)))

static bool is_pin_name_valid(pin_name_t pin_name)
{
	port_id_t port = PD_GET_PORT(pin_name);
	pin_id_t pin = PD_GET_PIN(pin_name);

	switch (port) {
	case PORT_A:
	case PORT_B:
	case PORT_C:
		if (pin >= NUM_PINS_PORT_A_TO_C)
			return false;
		break;
	case PORT_D:
		if (pin >= NUM_PINS_PORT_D)
			return false;
		break;
	case PORT_H:
		if (pin >= NUM_PINS_PORT_H)
			return false;
		break;
	default:
		return false;
	}
	return true;
}

void pp_mark_pin_used(pin_name_t pin_name)
{
	if (!is_pin_name_valid(pin_name))
		return;

	port_id_t port = PD_GET_PORT(pin_name);
	pin_id_t pin = PD_GET_PIN(pin_name);

	MARK_AS_USED(port, pin);
}

bool pp_is_pin_used(pin_name_t pin_name)
{
	if (!is_pin_name_valid(pin_name))
		return false;

	port_id_t port = PD_GET_PORT(pin_name);
	pin_id_t pin = PD_GET_PIN(pin_name);

	return QUERY_USAGE(port, pin);
}

/* Enable the clock for the port */
static void enable_gpio_port_clock(port_id_t port)
{
	switch (port) {
	case PORT_A:
		__HAL_RCC_GPIOA_CLK_ENABLE();
		break;
	case PORT_B:
		__HAL_RCC_GPIOB_CLK_ENABLE();
		break;
	case PORT_C:
		__HAL_RCC_GPIOC_CLK_ENABLE();
		break;
	case PORT_D:
		__HAL_RCC_GPIOD_CLK_ENABLE();
		break;
	case PORT_H:
		__HAL_RCC_GPIOH_CLK_ENABLE();
		break;
	default:
		break;
	}
}

/* Ensure that the pin can be mapped to the peripheral */
static size_t can_pin_be_mapped(pin_name_t pin_name, const pin_map_t *mapping)
{
	size_t idx = 0;
	while (mapping[idx].pin_name != NC) {
		if (mapping[idx].pin_name == pin_name)
			return idx;
		idx++;
	}
	return NC;
}

/*
 * Map the mode settings of a pin attached to a peripheral to the underlying
 * representation used by the driver.
 */
static uint32_t get_hal_af_mode(const gpio_config_t *settings)
{
	switch (settings->pull_mode) {
	case OD_NO_PULL:
	case OD_PULL_UP:
	case OD_PULL_DOWN:
		return GPIO_MODE_AF_OD;
	case PP_NO_PULL:
	case PP_PULL_UP:
	case PP_PULL_DOWN:
		return GPIO_MODE_AF_PP;
	default:
		return NC;
	}
}

/*
 * Map the mode settings of a GPIO to the underlying representation used by
 * the driver.
 */
static uint32_t get_hal_mode(const gpio_config_t *settings)
{
	if (settings->dir == INPUT)
		return GPIO_MODE_INPUT;
	else if (settings->dir == OUTPUT)
		switch (settings->pull_mode) {
		case OD_NO_PULL:
		case OD_PULL_UP:
		case OD_PULL_DOWN:
			return GPIO_MODE_OUTPUT_OD;
		case PP_NO_PULL:
		case PP_PULL_UP:
		case PP_PULL_DOWN:
			return GPIO_MODE_OUTPUT_PP;
		default:
			return NC;
		}
	else
		return NC;
}

/*
 * Map the pull settings of a GPIO to the underlying representation used by
 * the driver.
 */
static uint32_t get_hal_pull(const gpio_config_t *settings)
{
	switch (settings->pull_mode) {
	case OD_NO_PULL:
	case PP_NO_PULL:
		return GPIO_NOPULL;
	case OD_PULL_UP:
	case PP_PULL_UP:
		return GPIO_PULLUP;
	case OD_PULL_DOWN:
	case PP_PULL_DOWN:
		return GPIO_PULLDOWN;
	default:
		return NC;
	}
}

/*
 * Map the speed settings of the pin to the underlying representation used by
 * the driver.
 */
static uint32_t get_hal_speed(const gpio_config_t *settings)
{
	switch (settings->speed) {
	case SPEED_LOW:
		return GPIO_SPEED_FREQ_LOW;
	case SPEED_MEDIUM:
		return GPIO_SPEED_FREQ_MEDIUM;
	case SPEED_HIGH:
		return GPIO_SPEED_FREQ_HIGH;
	case SPEED_VERY_HIGH:
		return GPIO_SPEED_FREQ_VERY_HIGH;
	default:
		return NC;
	}
}

periph_t pp_get_peripheral(pin_name_t pin_name, const pin_map_t *mapping)
{
	if (!is_pin_name_valid(pin_name))
		return NC;

	size_t idx = can_pin_be_mapped(pin_name, mapping);
	if (idx == NC)
		return NC;

	return mapping[idx].peripheral;
}

#define RET_ON_NOT_FOUND(val)	do { \
	if ((val) == NC) \
	return false; \
} while(0)

bool pp_peripheral_pin_init(pin_name_t pin_name, const pin_map_t *mapping)
{
	if (!is_pin_name_valid(pin_name))
		return false;

	if (pp_is_pin_used(pin_name))
		return false;

	size_t idx = can_pin_be_mapped(pin_name, mapping);
	RET_ON_NOT_FOUND(idx);

	port_id_t port = PD_GET_PORT(pin_name);
	pin_id_t pin = PD_GET_PIN(pin_name);

	/* Initialize pin for peripheral */
	GPIO_InitTypeDef gpio_pin;
	const gpio_config_t *settings = &mapping[idx].settings;
	enable_gpio_port_clock(port);
	gpio_pin.Pin = pp_map_drv_pin(pin);

	uint32_t value = get_hal_af_mode(settings);
	RET_ON_NOT_FOUND(value);
	gpio_pin.Mode = value;

	value = get_hal_pull(settings);
	RET_ON_NOT_FOUND(value);
	gpio_pin.Pull = value;

	value = get_hal_speed(settings);
	RET_ON_NOT_FOUND(value);
	gpio_pin.Speed = value;

	gpio_pin.Alternate = mapping[idx].alt_func;

	HAL_GPIO_Init((GPIO_TypeDef *)pp_map_drv_port(pin_name), &gpio_pin);

	MARK_AS_USED(port, pin);
	return true;
}

bool pp_gpio_pin_init(pin_name_t pin_name, const gpio_config_t *settings)
{
	if (!is_pin_name_valid(pin_name))
		return false;

	if (pp_is_pin_used(pin_name))
		return false;

	port_id_t port = PD_GET_PORT(pin_name);
	pin_id_t pin = PD_GET_PIN(pin_name);

	/* Initialize pin as GPIO */
	GPIO_InitTypeDef gpio_pin;
	enable_gpio_port_clock(port);
	gpio_pin.Pin = pp_map_drv_pin(pin);

	uint32_t value = get_hal_mode(settings);
	RET_ON_NOT_FOUND(value);
	gpio_pin.Mode = value;

	value = get_hal_pull(settings);
	RET_ON_NOT_FOUND(value);
	gpio_pin.Pull = value;

	value = get_hal_speed(settings);
	RET_ON_NOT_FOUND(value);
	gpio_pin.Speed = value;

	HAL_GPIO_Init((GPIO_TypeDef *)pp_map_drv_port(pin_name), &gpio_pin);

	MARK_AS_USED(port, pin);
	return true;
}

uint32_t pp_map_drv_port(pin_name_t pin_name)
{
	if (!is_pin_name_valid(pin_name))
		return (uint32_t)NULL;

	port_id_t port = PD_GET_PORT(pin_name);

	switch (port) {
	case PORT_A:
		return (uint32_t)GPIOA;
	case PORT_B:
		return (uint32_t)GPIOB;
	case PORT_C:
		return (uint32_t)GPIOC;
	case PORT_D:
		return (uint32_t)GPIOD;
	case PORT_H:
		return (uint32_t)GPIOH;
	default:
		return (uint32_t)NULL;
	}
}

uint32_t pp_map_drv_pin(pin_name_t pin_name)
{
	if (!is_pin_name_valid(pin_name))
		return NC;

	pin_id_t pin = PD_GET_PIN(pin_name);

	return (uint16_t)(1 << pin);
}
