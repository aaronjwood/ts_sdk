/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "timer_hal.h"
#include <stdlib.h>

#define CHECK_RET_VALID_INTERFACE(interface, retval) \
	if ((interface) == NULL) \
		return retval

#define CHECK_VALID_INTERFACE(interface) \
	if ((interface) == NULL) \
		return

bool timer_init(const timer_interface_t *const inst_interface,
		uint32_t period, uint32_t priority, uint32_t base_freq,
		timercallback_t callback)
{
	CHECK_RET_VALID_INTERFACE(inst_interface, false);
	if (!inst_interface->init_timer(period, priority, base_freq,
		inst_interface->data))
		return false;
	inst_interface->reg_callback(callback, inst_interface->data);
	return true;
}

bool timer_is_running(const timer_interface_t *const inst_interface)
{
	CHECK_RET_VALID_INTERFACE(inst_interface, false);
	return inst_interface->is_running(inst_interface->data);
}

void timer_start(const timer_interface_t *const inst_interface)
{
	CHECK_VALID_INTERFACE(inst_interface);
	inst_interface->start(inst_interface->data);
}

uint32_t timer_get_time(const timer_interface_t *const inst_interface)
{
	CHECK_RET_VALID_INTERFACE(inst_interface, 0);
	return inst_interface->get_time(inst_interface->data);
}

void timer_set_time(const timer_interface_t *const inst_interface,
		uint32_t period)
{
	CHECK_VALID_INTERFACE(inst_interface);
	inst_interface->set_time(period, inst_interface->data);
}

void timer_stop(const timer_interface_t *const inst_interface)
{
	CHECK_VALID_INTERFACE(inst_interface);
	inst_interface->stop(inst_interface->data);
}

void timer_irq_handler(const timer_interface_t *const inst_interface)
{
	CHECK_VALID_INTERFACE(inst_interface);
	inst_interface->irq_handler(inst_interface->data);
}
