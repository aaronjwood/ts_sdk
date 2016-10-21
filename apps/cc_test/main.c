/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include "dbg.h"
#include "cloud_comm.h"
#include "platform.h"

CC_RECV_BUFFER(send_buffer, OTT_DATA_SZ);
int main(int argc, char *argv[])
{
	platform_init();

	dbg_module_init();

	dbg_printf("Begin:\n");
	while (1) {
	}
	return 0;
}
