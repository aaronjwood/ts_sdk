/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>
#include "dbg.h"
#include "utils.h"
#include "oem_hal.h"
#include "sys.h"

/*
 * This test program is an illustration on how to use oem layer.
 */

int main(void)
{
	sys_init();
	dbg_module_init();
	oem_init();

	/* Get device info profile */
	char *temp = oem_get_profile_info_in_json("DINF", true);
	dbg_printf("%s\n", temp);
	free(temp);

	/* Get specific characterisitics */
	temp = oem_get_characteristic_info_in_json("Kernel Version", false);
	dbg_printf("%s\n", temp);
	free(temp);
	return 0;
}
