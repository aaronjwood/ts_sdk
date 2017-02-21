/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

/*
 * Minimal OTT application program. It sends back NUM_STATUS messages to the
 * cloud. If an update message is received, it is made the new status message.
 * Each status message can be at most 10 bytes large.
 */
#include <string.h>

#include "platform.h"
#include "cloud_comm.h"
#include "cc_control_service.h"
#include "dbg.h"
#include "dev_creds.h"

CC_SEND_BUFFER(send_buffer, CC_MAX_SEND_BUF_SZ);
CC_RECV_BUFFER(recv_buffer, CC_MAX_RECV_BUF_SZ);

static uint8_t status[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

static cc_data_sz send_data_sz = sizeof(status);

#define SERVER_NAME	"iwk.ott.thingspace.verizon.com"
#define SERVER_PORT	"443"
#define NUM_STATUSES	((uint8_t)4)

/* Receive callback */
static void recv_cb(cc_buffer_desc *buf, cc_event event, cc_service_id svc_id)
{
	dbg_printf("\t\t[RECV CB] Received an event for service %d.\n", svc_id);
	if (svc_id != CC_SERVICE_BASIC)
		cc_dispatch_event_to_service(svc_id, buf, event);
	else {

		if (event == CC_EVT_RCVD_MSG) {
			cc_data_sz sz = cc_get_receive_data_len(buf);
			const uint8_t *recvd = cc_get_recv_buffer_ptr(buf);

			dbg_printf("\t\t\t");
			dbg_printf("Update Message: \n");
			for (cc_data_sz i = 0; i < sz; i++)
				dbg_printf("\t\t\t\t[Byte %u]: 0x%x\n", i,
					   recvd[i]);

			/*
			 * For this example, replace the status message with
			 * the newly received update message.
			 */
			send_data_sz = (sz > sizeof(status)) ?
				sizeof(status) : sz;
			memcpy(status, recvd, send_data_sz);
		} else
			dbg_printf("\t\t\tUnexpected event: %d\n", event);

		/* ACK all application messages by default */
		dbg_printf("\t\t[RECV CB] Will send an ACK in response\n");
		cc_ack_msg();
	}

	/* Reschedule a receive */
	cc_recv_result s = cc_recv_msg_from_cloud(&recv_buffer, recv_cb);
	ASSERT(s == CC_RECV_SUCCESS || s == CC_RECV_BUSY);
}

/* Handle events related to Control service messages received from the cloud. */
static void ctrl_cb(cc_event event, uint32_t value, void *ptr)
{
	dbg_printf("\t\t[CTRL CB] Received an event.\n");
	if (event == CC_EVT_CTRL_SLEEP)
		dbg_printf("\t\t\tSleep interval (secs): %"PRIu32"\n", value);
	else
		dbg_printf("\t\t\tUnsupported control event: %d\n", event);
}

/* Send callback */
static void send_cb(cc_buffer_desc *buf, cc_event event, cc_service_id svc_id)
{
	if (svc_id != CC_SERVICE_BASIC)
		cc_dispatch_event_to_service(svc_id, buf, event);
	else {
		if (event == CC_EVT_SEND_ACKED)
			dbg_printf("\t\t[SEND CB] Received an ACK\n");
		else if (event == CC_EVT_SEND_NACKED)
			dbg_printf("\t\t[SEND CB] Received a NACK\n");
		else if (event == CC_EVT_SEND_TIMEOUT)
			dbg_printf("\t\t[SEND CB] Timed out trying to send "
				   "message\n");
	}

	/* Reschedule a receive */
	cc_recv_result s = cc_recv_msg_from_cloud(&recv_buffer, recv_cb);
	ASSERT(s == CC_RECV_SUCCESS || s == CC_RECV_BUSY);
}

int main(int argc, char *argv[])
{
	platform_init();
	dbg_module_init();
	dbg_printf("Begin:\n");

	dbg_printf("Initializing communications module\n");
	ASSERT(cc_init(ctrl_cb));

	dbg_printf("Setting remote destination\n");
	ASSERT(cc_set_destination(SERVER_NAME ":" SERVER_PORT));

	dbg_printf("Setting device authentiation credentials\n");
	ASSERT(cc_set_auth_credentials(d_ID, sizeof(d_ID),
				d_sec, sizeof(d_sec)));

	int32_t next_wakeup_interval = -1;	/* Interval value in ms */
	uint32_t cur_ts;			/* Current timestamp in ms */
	int32_t wake_up_interval = 15000;	/* Interval value in ms */

	dbg_printf("Setting initial value of status message\n");
	uint8_t *send_dptr = cc_get_send_buffer_ptr(&send_buffer);
	memcpy(send_dptr, status, sizeof(status));

	dbg_printf("Beginning CC API test\n\n");
	dbg_printf("Scheduling a receive\n");
	ASSERT(cc_recv_msg_from_cloud(&recv_buffer, recv_cb) == CC_RECV_SUCCESS);

	/*
	 * Let the cloud services know the device is powering up, possibly after
	 * a restart. This call is redundant when the device hasn't been activated
	 * since the net effect will be to receive the initial configuration and
	 * polling interval twice.
	 */
	dbg_printf("Sending \"restarted\" message\n");
	ASSERT(cc_ctrl_resend_init_config(send_cb) == CC_SEND_SUCCESS);

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
