/* Copyright(C) 2017 Verizon. All rights reserved. */

/*
 * Example firmware that reports device information when it receives command to
 * do so (for mqtt enabled devices its GetOtp) and also on activation request
 *
 * To keep the test program simple, error handling is kept to a minimum.
 * Wherever possible, the program halts with an ASSERT in case the API fails.
 */

#include "sys.h"
#include "cloud_comm.h"
#include "cc_basic_service.h"
#include "cc_control_service.h"
#include "dbg.h"
#include "protocol_init.h"
#include "rcvd_msg.h"


/* Declare the send and receive static buffer this is mandatory step */
CC_SEND_BUFFER(send_buffer, CC_MAX_SEND_BUF_SZ);
CC_RECV_BUFFER(recv_buffer, CC_MAX_RECV_BUF_SZ);

/* Number of times to retry sending in case of failure */
#define MAX_RETRIES	((uint8_t)3)

/* Arbitrary long sleep time in milliseconds */
#define LONG_SLEEP_INT_MS	180000

#if 0
static void send_with_retry(cc_buffer_desc *b, cc_data_sz s, cc_service_id id)
{
	uint8_t retries = 0;
	cc_send_result res;
	while (retries < MAX_RETRIES) {
		res = cc_send_svc_msg_to_cloud(b, s, id);
		if (res == CC_SEND_SUCCESS)
			break;
		retries++;
		dbg_printf("\t%s: send attempt %d out of %d failed\n", __func__,
				retries, MAX_RETRIES);
	}
	if (res != CC_SEND_SUCCESS)
		dbg_printf("\t%s: Failed to send message.\n", __func__);
}

static uint32_t read_and_send_all_sensor_data(uint64_t cur_ts)
{
	if (last_st_ts != 0) {
		if ((cur_ts - last_st_ts) < STATUS_REPORT_INT_MS)
			return last_st_ts + STATUS_REPORT_INT_MS - cur_ts;
	}
	dbg_printf("Reading sensor data\n");
	array_t data;
	data.bytes = cc_get_send_buffer_ptr(&send_buffer, CC_SERVICE_BASIC);
	for (uint8_t i = 0; i < si_get_num_sensors(); i++) {
		ASSERT(si_read_data(i, SEND_DATA_SZ, &data));
		dbg_printf("\tSensor [%d], ", i);
		dbg_printf("sending out status message : %d bytes\n", data.sz);
		send_with_retry(&send_buffer, data.sz, CC_SERVICE_BASIC);
	}
	last_st_ts = cur_ts;
	return STATUS_REPORT_INT_MS;
}
#endif

static void receive_completed(cc_buffer_desc *buf)
{
	cc_data_sz sz = cc_get_receive_data_len(buf, CC_SERVICE_BASIC);
	const char *recvd = (const char *)cc_get_recv_buffer_ptr(buf,
				CC_SERVICE_BASIC);
	rsp rsp_to_remote;
	char *rsp_msg = NULL;
	if (recvd && sz > 0)
		process_rvcd_msg(recvd, sz, &rsp_to_remote);
	printf("Response created......\n");
	if (rsp_to_remote.rsp_msg)
		printf("%s\n", rsp_msg);
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
	else if (event == CC_EVT_RCVD_WRONG_MSG)
		dbg_printf("\t\t\tWrong message received\n");
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

int main(void)
{
	uint32_t next_wakeup_interval = 0;	/* Interval value in ms */
	uint32_t wake_up_interval = 15000;	/* Interval value in ms */
	uint32_t slept_till = 0;

	sys_init();
	dbg_module_init();
	dbg_printf("Begin:\n");

	dbg_printf("Initializing communications module\n");
	ASSERT(cc_init(ctrl_cb));

	dbg_printf("Register to use the Basic service\n");
	ASSERT(cc_register_service(&cc_basic_service_descriptor,
				   basic_service_cb));

	dbg_printf("Setting remote host and port\n");
	ASSERT(cc_set_destination(REMOTE_HOST));

	dbg_printf("Setting device authentiation credentials\n");
	ASSERT(cc_set_own_auth_credentials(cl_cred, CL_CRED_SZ,
				cl_sec_key, CL_SEC_KEY_SZ));
	dbg_printf("Setting remote side authentiation credentials\n");
	ASSERT(cc_set_remote_credentials(cacert, CA_CRED_SZ));

	/* This step is mandatory */
	dbg_printf("Activate receive buffer to receive communications\n");
	ASSERT(cc_set_recv_buffer(&recv_buffer) == CC_RECV_SUCCESS);

	while (1) {
		next_wakeup_interval = cc_service_send_receive(
							sys_get_tick_ms());
		if (next_wakeup_interval == 0) {
			wake_up_interval = LONG_SLEEP_INT_MS;
			dbg_printf("Protocol does not required to be called"
				",sleeping for %"PRIu32" sec.\n",
				wake_up_interval / 1000);
		} else {
			dbg_printf("Protocol requests wakeup in %"
				   PRIu32" sec.\n", next_wakeup_interval /1000);
			wake_up_interval = next_wakeup_interval;
		}

		dbg_printf("Reporting required in %"PRIu32" sec.\n",
			wake_up_interval / 1000);
		dbg_printf("Powering down for %"PRIu32" seconds\n\n",
				wake_up_interval / 1000);
		slept_till = sys_sleep_ms(wake_up_interval);
		slept_till = wake_up_interval - slept_till;
		dbg_printf("Slept for %"PRIu32" seconds\n\n", slept_till / 1000);
	}
	return 0;
}
