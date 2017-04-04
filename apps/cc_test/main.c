/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

/*
 * Minimal OTT application program. It sends back NUM_STATUS messages to the
 * cloud. If an update message is received, it is made the new status message.
 * Each status message can be at most 10 bytes large.
 */
#include <string.h>

#include "platform.h"
#include "cloud_comm.h"
#include "cc_basic_service.h"
#include "cc_control_service.h"
#include "dbg.h"
#include "dev_creds.h"

CC_SEND_BUFFER(send_buffer, CC_MAX_SEND_BUF_SZ);
CC_RECV_BUFFER(recv_buffer, CC_MAX_RECV_BUF_SZ);

#define CONCAT_SMS

#if defined (OTT_PROTOCOL)
#define REMOTE_HOST	"iwk.ott.thingspace.verizon.com:443"
static uint8_t status[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
#elif defined (SMSNAS_PROTOCOL)
#define REMOTE_HOST	"+12345678912"
#if defined (CONCAT_SMS)
static uint8_t status[500];
#else
static uint8_t status[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
#endif
#else
#error "define valid protocol options from OTT_PROTOCOL or SMSNAS_PROTOCOL"
#endif

static cc_data_sz send_data_sz = sizeof(status);

#define NUM_STATUSES	((uint8_t)1)

/* Arbitrary long sleep time in milliseconds */
#define LONG_SLEEP_INT_MS	30000

/* status report interval in milliseconds */
#define STATUS_REPORT_INT_MS	15000
/* last status message sent timestamp */
uint64_t last_st_ts = 0;

static void receive_completed(cc_buffer_desc *buf)
{
	cc_data_sz sz = cc_get_receive_data_len(buf, CC_SERVICE_BASIC);
	const uint8_t *recvd = cc_get_recv_buffer_ptr(buf, CC_SERVICE_BASIC);

	dbg_printf("\t\t\t");
#if 0
	dbg_printf("Received Update Message: \n");
	for (cc_data_sz i = 0; i < sz; i++)
		dbg_printf("\t\t\t\t[Byte %u]: 0x%x, ", i, recvd[i]);
	dbg_printf("\n");
#endif

	/*
	 * For this example, replace the status message with
	 * the newly received update message.
	 */
	send_data_sz = (sz > sizeof(status)) ? sizeof(status) : sz;
	memcpy(status, recvd, send_data_sz);

	/* ACK all application messages by default */
	dbg_printf("\t\t\tACK'ing the message\n");
	cc_ack_msg();
}

static void handle_buf_overflow()
{
	dbg_printf("\t\t\tBuffer overflow\n");
	cc_nak_msg();
}

/*
 * Handle events related to the Basic service i.e. normal data messages to
 * and from the cloud.
 */
static void basic_service_cb(cc_event event, uint32_t value, void *ptr)
{
	dbg_printf("\t\t[BASIC CB] Received an event.\n");

	if (event == CC_EVT_RCVD_MSG)
		receive_completed((cc_buffer_desc *)ptr);

	else if (event == CC_EVT_SEND_ACKED)
		dbg_printf("\t\t\tReceived an ACK\n");

	else if (event == CC_EVT_SEND_NACKED)
		dbg_printf("\t\t\tReceived a NACK\n");

	else if (event == CC_EVT_SEND_TIMEOUT)
		dbg_printf("\t\t\tTimed out trying to send message\n");

	else if (event == CC_EVT_RCVD_OVERFLOW)
		handle_buf_overflow();
	else
		dbg_printf("\t\t\tUnexpected event received: %d\n", event);
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

static void set_send_buffer(bool init)
{
	if (init) {
#if defined (CONCAT_SMS) && defined (SMSNAS_PROTOCOL)
		memset(status, 1, 130);
		memset(status + 130, 2, 134);
		memset(status + 130 + 134, 3, 134);
		memset(status + 130 + 134 + 134, 4, 102);
#endif
	}
	uint8_t *send_dptr = cc_get_send_buffer_ptr(&send_buffer,
						    CC_SERVICE_BASIC);
	memcpy(send_dptr, status, send_data_sz);
}

static uint32_t send_status_msgs(uint64_t cur_ts)
{
	if (last_st_ts != 0) {
		if ((cur_ts - last_st_ts) < STATUS_REPORT_INT_MS)
			return last_st_ts + STATUS_REPORT_INT_MS - cur_ts;
	}
	dbg_printf("Sending out status messages\n");
	for (uint8_t i = 0; i < NUM_STATUSES; i++) {
		/* Mimics reading a value from the sensor */
		set_send_buffer(false);
		dbg_printf("\tStatus (%u/%u) of bytes: %u\n",
				i + 1, NUM_STATUSES, send_data_sz);
		ASSERT(cc_send_svc_msg_to_cloud(&send_buffer,
						send_data_sz,
						CC_SERVICE_BASIC)
		       == CC_SEND_SUCCESS);
	}
	last_st_ts = cur_ts;
	return STATUS_REPORT_INT_MS;
}

int main(int argc, char *argv[])
{
	platform_init();
	dbg_module_init();
	dbg_printf("Begin:\n");

	dbg_printf("Initializing communications module\n");
	ASSERT(cc_init(ctrl_cb));

	dbg_printf("Register to use the Basic service\n");
	ASSERT(cc_register_service(&cc_basic_service_descriptor,
				   basic_service_cb));

	dbg_printf("Setting remote destination\n");
	ASSERT(cc_set_destination(REMOTE_HOST));

	dbg_printf("Setting device authentiation credentials\n");
	ASSERT(cc_set_auth_credentials(d_ID, sizeof(d_ID),
				d_sec, sizeof(d_sec)));

	uint32_t next_wakeup_interval = 0;	/* Interval value in ms */
	uint32_t wake_up_interval = 15000;	/* Interval value in ms */
	uint32_t next_report_interval = 0;	/* Interval in ms */
	//uint32_t slept_till = 0;
	last_st_ts = 0;
	dbg_printf("Setting initial value of status message\n");
	set_send_buffer(true);
	dbg_printf("Beginning CC API test\n\n");
	dbg_printf("Make a receive buffer available\n");
	ASSERT(cc_set_recv_buffer(&recv_buffer) == CC_RECV_SUCCESS);

	/*
	 * Let the cloud services know the device is powering up, possibly after
	 * a restart. This call is redundant when the device hasn't been
	 * activated, since the net effect will be to receive the initial
	 * configuration twice.
	 */
	dbg_printf("Sending \"restarted\" message\n");
	ASSERT(cc_ctrl_resend_init_config() == CC_SEND_SUCCESS);
	while (1) {
		//printf("Current time stamp: %"PRIu32"\n",
		//	(uint32_t)platform_get_tick_ms() / 1000);
		next_report_interval = send_status_msgs(platform_get_tick_ms());
		next_wakeup_interval = cc_service_send_receive(
						platform_get_tick_ms());
		if (next_wakeup_interval == 0) {
			wake_up_interval = LONG_SLEEP_INT_MS;
			//dbg_printf("Protocol does not required to be called"
			//	",sleeping for %"PRIu32" sec.\n",
			//	wake_up_interval / 1000);
		} else {
			//dbg_printf("Protocol requests wakeup in %"
			//	   PRIu32" sec.\n", next_wakeup_interval / 1000);
			wake_up_interval = next_wakeup_interval;
		}

		if (wake_up_interval > next_report_interval) {
			wake_up_interval = next_report_interval;
			//dbg_printf("Reporting required in %"
			//	   PRIu32" sec.\n", wake_up_interval / 1000);
		}

		//dbg_printf("Powering down for %"PRIu32" seconds\n\n",
		//		wake_up_interval / 1000);
		//slept_till = platform_sleep_ms(wake_up_interval);
		//slept_till = wake_up_interval - slept_till;
		//dbg_printf("Slept for %"PRIu32" seconds\n\n", slept_till / 1000);
	}
	return 0;
}
