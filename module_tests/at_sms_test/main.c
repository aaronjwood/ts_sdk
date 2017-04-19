/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

/*
 * This example program tests the SMSNAS AT layer. The program repeatedly sends
 * a single part SMS followed by a two part SMS every 30 seconds. Any SMS
 * segments it receives is printed out along with its metadata.
 */
#include <string.h>
#include "dbg.h"
#include "at_sms.h"
#include "sys.h"

#define WAIT_TIME_SEC	((uint8_t)30)
#define MAX_ACK_TRIES	((uint8_t)5)

static volatile bool ack_pending;

/* Print information about the received message segment */
static void rcv_cb(const at_msg_t *sms_seg)
{
	dbg_printf("\nSource Addr: %s\n", sms_seg->addr);
	dbg_printf("Ref. No. = %u\n", sms_seg->ref_no);
	dbg_printf("No. parts = %u\n", sms_seg->num_seg);
	dbg_printf("Seq. No. = %u\n", sms_seg->seq_no);
	dbg_printf("Len = %u\n", sms_seg->len);
	dbg_printf("Data:\n");
	for (uint8_t i = 0; i < sms_seg->len; i++)
		dbg_printf("%c", sms_seg->buf[i]);
	dbg_printf("\n");

	ack_pending = true;
}

/* Wait for 'ms' milliseconds, ACKing any messages received */
static void wait_ms(uint32_t ms)
{
	uint32_t start = platform_get_tick_ms();
	uint32_t end = start;
	uint8_t num_tries = 0;
	do {
		if (ack_pending) {
			dbg_printf("Sending ACK in response\n");
			ack_pending = false;
			bool ack_sent = at_sms_ack();
			num_tries++;
			if (!ack_sent) {
				dbg_printf("Error: Unable to issue an ACK\n");
				if (num_tries <= MAX_ACK_TRIES)
					ack_pending = true;
			}
		}
		end = platform_get_tick_ms();
	} while(end - start <= ms);
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

	/* When testing in the lab, the destination is the same regardless of number. */
	char num[ADDR_SZ + 1] = {"+11234567890"};
	printf("SIM Number : %s\n", num);

	while (1) {
		/* Send single part SMS to destination */
		uint8_t payload[] = "This is a test.";
		at_msg_t outgoing_msg = {
			.len = sizeof(payload),
			.buf = payload,
			.ref_no = 42,
			.num_seg = 1,
			.seq_no = 0,
			.addr = num
		};

		dbg_printf("Sending a single part message (%s)\n", num);
		if (!at_sms_send(&outgoing_msg)) {
			dbg_printf("Error sending message\n");
			goto done;
		}

		platform_delay(2500);

		/* Send multi part SMS to destination */
		dbg_printf("Sending a multi-part message (%s)\n", num);
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

		dbg_printf("Waiting for %u seconds\n", WAIT_TIME_SEC);
		wait_ms(WAIT_TIME_SEC * 1000);
	}

done:
	dbg_printf("Test completed\n");
	while (1)
		;
	return 0;
}
