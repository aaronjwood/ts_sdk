/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "sys.h"
#include "dbg.h"
#include "sensor_i2c_interface.h"

int main()
{
	sys_init();
	array_t data;
	dbg_module_init();
	dbg_printf("Begin:\n");
	
	dbg_printf("Initializing the sensors\n");
	ASSERT(si_i2c_init());
	
	dbg_printf("Reading sensor data\n");
	ASSERT(si_i2c_read_data(&data));

	return 0;
}
