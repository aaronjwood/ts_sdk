/* Copyright(C) 2016 Verizon. All rights reserved. */

/*
 * Minimal OTT application program. It sends back NUM_STATUS messages to the
 * cloud. If an update message is received, it is made the new status message.
 * Each status message can be at most 10 bytes large.
 */
#include <string.h>

#include "platform.h"
#include "cloud_comm.h"
#include "dbg.h"
#include "dev_creds.h"

CC_SEND_BUFFER(send_buffer, PROTO_DATA_SZ);
CC_RECV_BUFFER(recv_buffer, PROTO_DATA_SZ);

#define DEVICE_UUID_SZ_BYTES	16
#define STATUS_SZ		10
static uint8_t status[STATUS_SZ] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static cc_data_sz send_data_sz = sizeof(status);

#define SERVER_NAME	"iwk.ott.thingspace.verizon.com"
#define SERVER_PORT	"443"
#define NUM_STATUSES	((uint8_t)4)

/* Receive callback */
static void recv_cb(const void *buf, uint32_t sz, cc_event event)
{
	dbg_printf("\t\t[RECV CB] Received a message:\n");
	if (event == CC_STS_RCV_UPD) {
		uint8_t *temp_buf = (uint8_t *)buf;
		dbg_printf("\t\t\t");
		dbg_printf("Update Message: \n");
		for (uint32_t i = 0; i < sz; i++)
			dbg_printf("\t\t\t\t[Byte %lu]: 0x%x\n", i, temp_buf[i]);
		/* Make the new status message to be the received update message */
		send_data_sz = (sz > sizeof(status)) ? sizeof(status) : sz;
		memcpy(status, buf, send_data_sz);
	} else if (event == CC_STS_RCV_CMD_SL) {
		uint32_t sl_sec = cc_get_sleep_interval(buf);
		dbg_printf("\t\t\tSleep interval (secs): %"PRIu32"\n", sl_sec);
	}

	/* ACK all messages by default */
	dbg_printf("\t\t[RECV CB] Will send an ACK in response\n");
	cc_ack_bytes();
	/* Reschedule a receive */
	cc_recv_result s = cc_recv_bytes_from_cloud(&recv_buffer, recv_cb);
	ASSERT(s == CC_RECV_SUCCESS || s == CC_RECV_BUSY);
}

/* Send callback */
static void send_cb(const void *buf, uint32_t sz, cc_event event)
{
	if (event == CC_STS_ACK)
		dbg_printf("\t\t[SEND CB] Received an ACK\n");
	else if (event == CC_STS_NACK)
		dbg_printf("\t\t[SEND CB] Received a NACK\n");
	else if (event == CC_STS_SEND_TIMEOUT)
		dbg_printf("\t\t[SEND CB] Timed out trying to send message\n");

	/* Reschedule a receive */
	cc_recv_result s = cc_recv_bytes_from_cloud(&recv_buffer, recv_cb);
	ASSERT(s == CC_RECV_SUCCESS || s == CC_RECV_BUSY);
}

int main(int argc, char *argv[])
{
	platform_init();
	dbg_module_init();
	dbg_printf("Begin:\n");

	dbg_printf("Initializing communications module\n");
	ASSERT(cc_init());

	dbg_printf("Setting remote host and port\n");
	ASSERT(cc_set_destination(SERVER_NAME, SERVER_PORT));

	dbg_printf("Setting device authentiation credentials\n");
	ASSERT(cc_set_auth_credentials(d_ID, DEVICE_UUID_SZ_BYTES,
		d_sec, sizeof(d_sec)));

	int32_t next_wakeup_interval = -1;	/* Interval value in ms */
	uint32_t cur_ts;			/* Current timestamp in ms */
	int32_t wake_up_interval = 15000;	/* Interval value in ms */

	dbg_printf("Setting initial value of status message\n");
	uint8_t *send_dptr = cc_get_send_buffer_ptr(&send_buffer);
	memcpy(send_dptr, status, sizeof(status));

	dbg_printf("Beginning CC API test\n\n");
	dbg_printf("Scheduling a receive\n");
	ASSERT(cc_recv_bytes_from_cloud(&recv_buffer, recv_cb) == CC_RECV_SUCCESS);

	/*
	 * Let the cloud services know the device is powering up, possibly after
	 * a restart. This call is redundant when the device hasn't been activated
	 * since the net effect will be to receive the initial configuration and
	 * polling interval twice.
	 */
	dbg_printf("Sending \"restarted\" message\n");
	ASSERT(cc_resend_init_config(send_cb) == CC_SEND_SUCCESS);

	while (1) {
		dbg_printf("Sending out status messages\n");
		for (uint8_t i = 0; i < NUM_STATUSES; i++) {
			/* Mimics reading a value from the sensor */
			uint8_t *send_dptr = cc_get_send_buffer_ptr(&send_buffer);
			memcpy(send_dptr, status, send_data_sz);

			dbg_printf("\tStatus (%u/%u)\n",
					i + 1, NUM_STATUSES);
			ASSERT(cc_send_msg_to_cloud(&send_buffer,
						send_data_sz,
						send_cb) == CC_SEND_SUCCESS);
		}

		cur_ts = platform_get_tick_ms();
		next_wakeup_interval = cc_service_send_receive(cur_ts);
		if (next_wakeup_interval != -1) {
			if (wake_up_interval != next_wakeup_interval)
				dbg_printf("New wakeup time received\n");
			dbg_printf("Relative wakeup time set to +%"PRIi32" sec\n",
					next_wakeup_interval / 1000);
			wake_up_interval = next_wakeup_interval;
		}

		dbg_printf("Powering down for %"PRIi32" seconds\n\n",
				wake_up_interval / 1000);
		platform_delay(wake_up_interval);
	}
	return 0;
}
