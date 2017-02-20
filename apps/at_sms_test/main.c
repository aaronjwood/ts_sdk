/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include <string.h>
#include "dbg.h"
#include "at_sms.h"
#include "platform.h"

int main(int argc, char *argv[])
{
	platform_init();
	dbg_module_init();
	dbg_printf("Begin:\n");
	dbg_printf("Initializing modem...\n");

	if (!at_init()) {
		dbg_printf("Couldn't initialize modem!\n");
		goto done;
	}

done:
	dbg_printf("Test completed\n");
	while (1)
		;
	return 0;
}
