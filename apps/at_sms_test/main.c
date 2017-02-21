/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include <string.h>
#include "dbg.h"
#include "at_sms.h"
#include "platform.h"

static void rcv_cb(at_msg_t *sms_seg)
{
	/* Code */
}

int main(int argc, char *argv[])
{
	platform_init();
	dbg_module_init();
	dbg_printf("Begin:\n");
	dbg_printf("Initializing modem\n");

	at_sms_set_rcv_cb(rcv_cb);
	if (!at_init()) {
		dbg_printf("Couldn't initialize modem!\n");
		goto done;
	}

	/* TODO: Send single part SMS to self */

	/* TODO: Send multi part SMS to self */

done:
	dbg_printf("Test completed\n");
	while (1)
		;
	return 0;
}
