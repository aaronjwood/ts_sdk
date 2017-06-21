/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include "sys.h"
#include "dbg.h"

#define SLEEP_INETRVAL 15000

/* To test timer functionality used sys_sleep_ms function.
It internally uses tim5 timer */
int main(int argc, char *argv[])
{
	sys_init();
	dbg_module_init();
	dbg_printf("Begin:\n");

	/* Interval value in ms */
	uint32_t sleep_interval = SLEEP_INETRVAL;
	uint32_t slept_till = 0;
	uint32_t timer_start_tick = 0;
	uint32_t timer_end_tick = 0;

	timer_start_tick = sys_get_tick_ms();
	dbg_printf("Started sleep timer:\n");
	slept_till = sys_sleep_ms(sleep_interval);
	timer_end_tick = sys_get_tick_ms();


	while (1) {
		if (slept_till) {
			dbg_printf("Interrupted: Slept till %"PRIu32" msec.\n"\
					, slept_till);
			sleep_interval = sleep_interval - slept_till;
		} else {
			dbg_printf("Uninterrupted: Slept till %"PRIu32"sec.\n"\
				, (timer_end_tick - timer_start_tick)/1000);
			sleep_interval = SLEEP_INETRVAL;
		}
		timer_start_tick = sys_get_tick_ms();
		slept_till = sys_sleep_ms(sleep_interval);
		timer_end_tick = sys_get_tick_ms();
	}
	return 0;
}
