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
	for (uint8_t i = 0; i < sms_seg->len; i++)
		dbg_printf("%c", sms_seg->buf[i]);
	dbg_printf("\n");

	/*
	 * XXX: ACK / NACK payload.
	 * They cannot be manually issued on the commercial network.
	 */
	/*if (!at_sms_ack())
	 *	dbg_printf("Error: Unable to issue an ACK\n");
	 */
}

int main(int argc, char *argv[])
{
	platform_init();
	dbg_module_init();
	dbg_printf("Begin:\n");
	dbg_printf("Initializing modem\n");

	at_sms_set_rcv_cb(rcv_cb);
	if (!at_init()) {
		dbg_printf("Couldn't initialize modem\n");
		goto done;
	}

	/* Retrieve the number associated with the SIM for a loopback test */
	char num[ADDR_SZ + 1];
	at_sms_retrieve_num(num);
	printf("SIM Number : %s\n", num);

	while (1) {
		/* Send single part SMS to self */
		uint8_t payload[] = "This is a test.";
		at_msg_t outgoing_msg = {
			.len = sizeof(payload),
			.buf = payload,
			.ref_no = 42,
			.num_seg = 1,
			.seq_no = 0,
			.addr = num
		};

		dbg_printf("Sending a single part message to self (%s)\n", num);
		if (!at_sms_send(&outgoing_msg)) {
			dbg_printf("Error sending message\n");
			goto done;
		}

		platform_delay(2500);

		/* Send multi part SMS to self */
		dbg_printf("Sending a multi-part message to self (%s)\n", num);
		uint8_t payload1[] = "First payload.";
		uint8_t payload2[] = "Second payload.";
		at_msg_t outgoing_segment1 = {
			.len = sizeof(payload1),
			.buf = payload1,
			.ref_no = 8,
			.num_seg = 2,
			.seq_no = 1,
			.addr = num
		};

		at_msg_t outgoing_segment2 = {
			.len = sizeof(payload2),
			.buf = payload2,
			.ref_no = 8,
			.num_seg = 2,
			.seq_no = 2,
			.addr = num
		};

		if (!at_sms_send(&outgoing_segment1)) {
			dbg_printf("Error sending segment 1\n");
			goto done;
		}

		if (!at_sms_send(&outgoing_segment2)) {
			dbg_printf("Error sending segment 2\n");
			goto done;
		}

		platform_delay(30000);
	}

done:
	dbg_printf("Test completed\n");
	while (1)
		;
	return 0;
}
