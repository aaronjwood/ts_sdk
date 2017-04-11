#include <stdint.h>
#include "port_pin_api.h"

/*
 * XXX: Chipset specific implementation, i.e. One implementation for the STM32F4
 * series, one for STM32L0 etc.
 */
#include <stm32f4xx_hal.h>

static port_size_t port_usage[NUM_PORTS];

#define MARK_AS_USED(port, pin)		(port_usage[(port)] |= (1 << (pin)))
#define QUERY_USAGE(port, pin)		((port_usage[(port)] | (1 << (pin))) == (1 << (pin)))

#define NOT_FOUND			((uint32_t)0xFFFFFFFF)

void pp_mark_pin_used(pin_name_t pin_name)
{
	if (!pd_is_pin_name_valid(pin_name))
		return;

	port_id_t port = pd_get_port(pin_name);
	pin_id_t pin = pd_get_pin(pin_name);

	MARK_AS_USED(port, pin);
}

bool pp_is_pin_used(pin_name_t pin_name)
{
	if (!pd_is_pin_name_valid(pin_name))
		return false;

	port_id_t port = pd_get_port(pin_name);
	pin_id_t pin = pd_get_pin(pin_name);

	return QUERY_USAGE(port, pin);
}

static inline void enable_gpio_port_clock(port_id_t port)
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
	case PORT_E:
		__HAL_RCC_GPIOE_CLK_ENABLE();
		break;
	case PORT_F:
		__HAL_RCC_GPIOF_CLK_ENABLE();
		break;
	case PORT_G:
		__HAL_RCC_GPIOG_CLK_ENABLE();
		break;
	case PORT_H:
		__HAL_RCC_GPIOH_CLK_ENABLE();
		break;
	default:
		break;
	}
}

static size_t can_pin_be_mapped(pin_name_t pin_name, const pin_t *mapping)
{
	size_t idx = 0;
	while (mapping[idx].pin_name != NC) {
		if (mapping[idx].pin_name == pin_name)
			return idx;
		idx++;
	}
	return NOT_FOUND;
}

static inline uint32_t get_hal_mode(const pin_t *map_elem)
{
	switch (map_elem->settings.pull_mode) {
	case OD_NO_PULL:
	case OD_PULL_UP:
	case OD_PULL_DOWN:
		return GPIO_MODE_AF_OD;
	case PP_NO_PULL:
	case PP_PULL_UP:
	case PP_PULL_DOWN:
		return GPIO_MODE_AF_PP;
	default:
		return NOT_FOUND;
	}
}

static inline uint32_t get_hal_pull(const pin_t *map_elem)
{
	switch (map_elem->settings.pull_mode) {
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
		return NOT_FOUND;
	}
}

static inline uint32_t get_hal_speed(const pin_t *map_elem)
{
	switch (map_elem->settings.speed) {
	case SPEED_LOW:
		return GPIO_SPEED_FREQ_LOW;
	case SPEED_MEDIUM:
		return GPIO_SPEED_FREQ_MEDIUM;
	case SPEED_HIGH:
		return GPIO_SPEED_FREQ_HIGH;
	case SPEED_VERY_HIGH:
		return GPIO_SPEED_FREQ_VERY_HIGH;
	default:
		return NOT_FOUND;
	}
}

static inline GPIO_TypeDef *get_hal_port(port_id_t port)
{
	switch (port) {
	case PORT_A:
		return GPIOA;
	case PORT_B:
		return GPIOB;
	case PORT_C:
		return GPIOC;
	case PORT_D:
		return GPIOD;
	case PORT_E:
		return GPIOE;
	case PORT_F:
		return GPIOF;
	case PORT_G:
		return GPIOG;
	case PORT_H:
		return GPIOH;
	default:
		return NULL;
	}
}

static inline uint16_t get_hal_pin(pin_id_t pin)
{
	return (uint16_t)(1 << pin);
}

bool pp_peripheral_pin_init(pin_name_t pin_name, const pin_t *mapping)
{
	if (!pd_is_pin_name_valid(pin_name))
		return false;

	if (pp_is_pin_used(pin_name))
		return false;

	size_t idx = can_pin_be_mapped(pin_name, mapping);
	if (idx == NOT_FOUND)
		return false;

	port_id_t port = pd_get_port(pin_name);
	pin_id_t pin = pd_get_pin(pin_name);

	/* Initialize pin for peripheral */
	GPIO_InitTypeDef gpio_pin;
	enable_gpio_port_clock(port);
	gpio_pin.Pin = get_hal_pin(pin);
	gpio_pin.Mode = get_hal_mode(&mapping[idx]);
	gpio_pin.Pull = get_hal_pull(&mapping[idx]);
	gpio_pin.Speed = get_hal_speed(&mapping[idx]);
	gpio_pin.Alternate = mapping[idx].alt_func;
	HAL_GPIO_Init(get_hal_port(port), &gpio_pin);

	MARK_AS_USED(port, pin);
	return true;
}
