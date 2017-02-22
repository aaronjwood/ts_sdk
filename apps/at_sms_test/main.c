/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include <string.h>
#include "dbg.h"
#include "at_sms.h"
#include "platform.h"

static void rcv_cb(const at_msg_t *sms_seg)
{
	dbg_printf("Source Addr: %s\n", sms_seg->addr);
	dbg_printf("Ref. No. = %u\n", sms_seg->ref_no);
	dbg_printf("No. parts = %u\n", sms_seg->num_seg);
	dbg_printf("Seq. No. = %u\n", sms_seg->seq_no);
	dbg_printf("Len = %u\n", sms_seg->len);
	dbg_printf("Data:\n");
	dbg_printf("%02X, ", sms_seg->buf[0]);
	for (uint8_t i = 1; i < sms_seg->len; i++)
		dbg_printf(", %02X", sms_seg->buf[i]);
	dbg_printf("\n");
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
