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
	char *temp = oem_get_profile_info_in_json("DINF");
	if (temp)
		dbg_printf("%s\n", temp);
	free(temp);

	/* Get specific characterisitics, kenrel version in this case */
	temp = oem_get_characteristic_info_in_json("KEV");
	if (temp)
		dbg_printf("%s\n", temp);
	free(temp);

	/* Get everything */
	temp = oem_get_all_profile_info_in_json(false);
	if (temp)
		dbg_printf("%s\n", temp);
	printf("Total Size: %d\n", strlen(temp));
	free(temp);
	return 0;
}
