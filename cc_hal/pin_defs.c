/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "pin_defs.h"
#include <stm32f4xx_hal.h>

/* Chipset specific implementation. Target: STM32F4xx */

port_id_t pd_get_port(pin_name_t pin_name)
{
	return ((port_id_t)(pin_name >> 4));
}

pin_id_t pd_get_pin(pin_name_t pin_name)
{
	return ((pin_id_t)(pin_name & 0x0F));
}

bool pd_is_pin_name_valid(pin_name_t pin_name)
{
	port_id_t port = pd_get_port(pin_name);
	pin_id_t pin = pd_get_pin(pin_name);

	switch (port) {
	case PORT_A:
	case PORT_B:
	case PORT_C:
	case PORT_D:
	case PORT_E:
	case PORT_F:
	case PORT_G:
		if (pin >= NUM_PINS_PORT_A_TO_G)
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

uint32_t pd_map_drv_port(pin_name_t pin_name)
{
	if (!pd_is_pin_name_valid(pin_name))
		return (uint32_t)NULL;

	port_id_t port = pd_get_port(pin_name);

	switch (port) {
	case PORT_A:
		return (uint32_t)GPIOA;
	case PORT_B:
		return (uint32_t)GPIOB;
	case PORT_C:
		return (uint32_t)GPIOC;
	case PORT_D:
		return (uint32_t)GPIOD;
	case PORT_E:
		return (uint32_t)GPIOE;
	case PORT_F:
		return (uint32_t)GPIOF;
	case PORT_G:
		return (uint32_t)GPIOG;
	case PORT_H:
		return (uint32_t)GPIOH;
	default:
		return (uint32_t)NULL;
	}
}

uint32_t pd_map_drv_pin(pin_name_t pin_name)
{
	if (!pd_is_pin_name_valid(pin_name))
		return NC;

	pin_id_t pin = pd_get_pin(pin_name);

	return (uint16_t)(1 << pin);
}
