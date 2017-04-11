#include "port_pin_api.h"

static port_size_t port_usage[NUM_PORTS];

#define MARK_AS_USED(port, pin)		(port_usage[(port)] |= (1 << (pin)))
#define MARK_AS_UNUSED(port, pin)	(port_usage[(port)] &= ~(1 << (pin)))
#define QUERY_USAGE(port, pin)		((port_usage[(port)] | (1 << (pin))) == (1 << (pin)))

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

bool pp_peripheral_pin_init(pin_name_t pin_name, const pin_t *mapping)
{
	if (!pd_is_pin_name_valid(pin_name))
		return false;

	port_id_t port = pd_get_port(pin_name);
	pin_id_t pin = pd_get_pin(pin_name);

	/* TODO: Initialize pin for peripheral */

	return true;
}
