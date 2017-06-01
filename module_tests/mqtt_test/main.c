/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include "dbg.h"
#include "sys.h"
#include "MQTTClient.h"

int main(int argc, char *argv[])
{
	sys_init();

	dbg_module_init();
	dbg_printf("Begin MQTT test:\n");

	while (1) {
	}

	return 0;
}
