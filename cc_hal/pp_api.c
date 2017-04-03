#include "pp_api.h"

/*
 * XXX: Chipset specific implementation, i.e. One implementation for the STM32F4
 * series, one for STM32L0 etc.
 */

static uint16_t port_usage[NUM_PORTS];

#define NUM_PINS_PORT_A_TO_G		16
#define NUM_PINS_PORT_H			2

#define MARK_AS_USED(port, pin)		(port_usage[port] |= (1 << pin))
#define MARK_AS_UNUSED(port, pin)	(port_usage[port] &= ~(1 << pin))
#define QUERY_USAGE(port, pin)		((port_usage[port] | (1 << pin)) == (1 << pin))

static bool is_valid_port_pin(port_id_t port, pin_id_t pin)
{
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

void pp_mark_pin_used(pin_name_t pin_name)
{
	port_id_t port = PD_GET_PORT(pin_name);
	pin_id_t pin = PD_GET_PIN(pin_name);

	if (!is_valid_port_pin(port, pin))
		return;

	MARK_AS_USED(port, pin);
}

bool pp_is_pin_used(pin_name_t pin_name)
{
	port_id_t port = PD_GET_PORT(pin_name);
	pin_id_t pin = PD_GET_PIN(pin_name);

	if (!is_valid_port_pin(port, pin))
		return false;

	return QUERY_USAGE(port, pin);
}

bool pp_peripheral_pin_init(pin_name_t pin_name, const pin_t *mapping)
{
	port_id_t port = PD_GET_PORT(pin_name);
	pin_id_t pin = PD_GET_PIN(pin_name);

	if (!is_valid_port_pin(port, pin))
		return false;

	/* TODO: Initialize pin for peripheral */

	return true;
}
