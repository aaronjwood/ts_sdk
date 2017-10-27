/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>
#include "port_pin_api.h"

/*
 * The following functions are just a stub, raspberry pi 3 does not support
 * any peripherals right now.
 *
 * Target board : Raspberry Pi 3
 * Target SoC   : BCM2837
 */


void pp_mark_pin_used(pin_name_t pin_name)
{

}

bool pp_is_pin_used(pin_name_t pin_name)
{
	return false;
}

periph_t pp_get_peripheral(pin_name_t pin_name, const pin_map_t *mapping)
{
	return NC;
}

bool pp_peripheral_pin_init(pin_name_t pin_name, const pin_map_t *mapping)
{
	return true;
}

bool pp_gpio_pin_init(pin_name_t pin_name, const gpio_config_t *settings)
{
	return true;
}

uint32_t pp_map_drv_port(pin_name_t pin_name)
{
	return 0;
}

uint32_t pp_map_drv_pin(pin_name_t pin_name)
{
	return 0;
}
