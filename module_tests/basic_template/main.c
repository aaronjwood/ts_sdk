/* Copyright(C) 2016 Verizon. All rights reserved. */

#include "sys.h"
#include "dbg.h"

int main(int argc, char *argv[])
{
	platform_init();

	dbg_module_init();

	while(1) {
		dbg_printf("This is an example of a moderately long sentence"
				"ending in a newline.\n");
		platform_delay(1000);
	}
	return 0;
}
