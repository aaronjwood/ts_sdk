/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "timer_hal.h"
#include <stdlib.h>

#define CHECK_VALID_INTERFACE(interface, retval) \
	if ((interface) == NULL) \
		return retval

bool timer_init(const timer_interface_t * const inst_interface,
		uint32_t period,
		timercallback_t callback)
{
	CHECK_VALID_INTERFACE(inst_interface, false);
	if (!inst_interface->init_period(period))
		return false;
	inst_interface->reg_callback(callback);
	return true;
}

bool timer_is_running(const timer_interface_t * const inst_interface)
{
	CHECK_VALID_INTERFACE(inst_interface, false);
	return inst_interface->is_running();
}

void timer_start(const timer_interface_t * const inst_interface)
{
	CHECK_VALID_INTERFACE(inst_interface, );
	inst_interface->start();
}

void timer_stop(const timer_interface_t * const inst_interface)
{
	CHECK_VALID_INTERFACE(inst_interface, );
	inst_interface->stop();
}
