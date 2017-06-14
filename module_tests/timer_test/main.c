/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include "sys.h"
#include "dbg.h"

int main(int argc, char *argv[])
{
	sys_init();
        dbg_module_init();
        dbg_printf("Begin:\n");
        
	uint32_t sleep_interval = 15000;      /* Interval value in ms */
        uint32_t slept_till = 0;
	uint32_t timer_start_tick = 0;
	uint32_t timer_end_tick = 0;
	bool b_slept_uninterrupted = false;
 
	timer_start_tick = sys_get_tick_ms();
	dbg_printf("Started sleep timer:\n");
	slept_till = sys_sleep_ms(sleep_interval);
	timer_end_tick = sys_get_tick_ms();
	
	if(slept_till) /* Intruppted during the sleep */
	{
		dbg_printf("Interrupted: Slept till %"PRIu32" msec.\n", slept_till);
	}
	else /* Slept unintrrupted till sleep_interval*/
	{
		dbg_printf("Uninterrupted: Slept till %"PRIu32" sec.\n",(timer_end_tick - timer_start_tick)/1000);
		b_slept_uninterrupted = true;
	}

	/* No interrupt during the sleep */  
      	ASSERT(b_slept_uninterrupted);
	return 0;
}
