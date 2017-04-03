/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stdio.h>
#include <stdlib.h>
#include "dbg.h"

bool __dbg_module_init(void)
{
	return true;
}

void raise_err(void)
{
	printf("raise_err() called. Aborting.\n");
	fflush(stdout);
	exit(1);
}
