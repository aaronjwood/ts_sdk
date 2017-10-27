/* Copyright(C) 2017 Verizon. All rights reserved. */

/* (for mqtt enabled devices only)
 * main source file, implementing cc API, it asks device to give back status
 * message if any at regular status interval and also wakes up device when
 * mqtt protocol requires to send keep alive ping or remote sends any data
 *
 */

#include <string.h>
#include "sys.h"
#include "cloud_comm.h"
#include "cc_basic_service.h"
#include "cc_control_service.h"
#include "dbg.h"
#include "protocol_init.h"
#include "rcvd_msg.h"
#include "oem_hal.h"
#include "utils.h"
#include "dev_profile_info.h"

#define DEV_ID_LEN     50
#define NET_INTFC      "eth0"

/* Declare the send and receive static buffer this is mandatory step */
CC_SEND_BUFFER(send_buffer, CC_MAX_SEND_BUF_SZ);
CC_RECV_BUFFER(recv_buffer, CC_MAX_RECV_BUF_SZ);

CC_SEND_BUFFER(status_buffer, CC_MAX_SEND_BUF_SZ);

/* Number of times to retry sending in case of failure */
#define MAX_RETRIES	((uint8_t)3)

static rsp rsp_to_remote;
static cc_data_sz send_sz;

/* Arbitrary long sleep time in milliseconds */
#define LONG_SLEEP_INT_MS	180000

/* Status message interval */
#define STATUS_REPORT_INT_MS	15000
/* last status message sent timestamp */
uint64_t last_st_ts = 0;

static void send_with_retry(cc_buffer_desc *b, cc_data_sz s, cc_service_id id,
				void *pub_topic, bool on_board)
{
	uint8_t retries = 0;
	cc_send_result res;
	while (retries < MAX_RETRIES) {
		if (on_board)
			res = cc_send_status_msg_to_cloud(b, s);
		else
			res = cc_send_svc_msg_to_cloud(b, s, id, pub_topic);

		if (res == CC_SEND_SUCCESS)
			break;
		retries++;
		dbg_printf("\t%s: send attempt %d out of %d failed\n", __func__,
				retries, MAX_RETRIES);
	}
	if (res != CC_SEND_SUCCESS)
		dbg_printf("\t%s: Failed to send message.\n", __func__);
}

static uint64_t send_status_msg()
{
	uint64_t status_int = LONG_SLEEP_INT_MS;
	uint64_t cur_ts = sys_get_tick_ms();
	if (last_st_ts != 0) {
		if ((cur_ts - last_st_ts) < STATUS_REPORT_INT_MS)
			return last_st_ts + STATUS_REPORT_INT_MS - cur_ts;
	}
	char *status_msg = read_device();
	if (!status_msg)
		goto done;
	else
		status_int = STATUS_REPORT_INT_MS;
	printf("status message created of size: %lu\n", strlen(status_msg));
	/* Now send this back this response */
	char *send_stat = (char *)cc_get_send_buffer_ptr(&status_buffer,
				CC_SERVICE_BASIC);
	memset(send_stat, 0, CC_MAX_SEND_BUF_SZ);
	cc_data_sz final_sz = (strlen(status_msg) <= CC_MAX_SEND_BUF_SZ) ?
		strlen(status_msg): CC_MAX_SEND_BUF_SZ;

	memcpy(send_stat, status_msg, final_sz);
	printf("Sending......\n");
	printf("%s\n", send_stat);
	cc_send_result res = cc_send_status_msg_to_cloud(&status_buffer,
				final_sz);
	if (res != CC_SEND_SUCCESS)
		printf("%s:%d: send failed\n", __func__, __LINE__);
done:
	last_st_ts = cur_ts;
	return status_int;
}

static void send_msg()
{

	send_with_retry(&send_buffer, send_sz, CC_SERVICE_BASIC,
			rsp_to_remote.uuid, rsp_to_remote.on_board);
	rsp_to_remote.valid_rsp = false;
}

static void receive_completed(cc_buffer_desc *buf)
{
	cc_data_sz sz = cc_get_receive_data_len(buf, CC_SERVICE_BASIC);
	const char *recvd = (const char *)cc_get_recv_buffer_ptr(buf,
				CC_SERVICE_BASIC);
	memset(rsp_to_remote.uuid, 0, MAX_CMD_SIZE);
	if (recvd && sz > 0)
		process_rvcd_msg(recvd, sz, &rsp_to_remote);
	if (rsp_to_remote.rsp_msg && (strlen(rsp_to_remote.rsp_msg) > 0)) {
		printf("Response created with size.....: %lu\n",
				strlen(rsp_to_remote.rsp_msg));
		/* Now send this back this response */
		char *send_rsp = (char *)cc_get_send_buffer_ptr(&send_buffer,
					CC_SERVICE_BASIC);
		memset(send_rsp, 0, CC_MAX_SEND_BUF_SZ);
		send_sz = (strlen(rsp_to_remote.rsp_msg) <= CC_MAX_SEND_BUF_SZ) ?
			strlen(rsp_to_remote.rsp_msg): CC_MAX_SEND_BUF_SZ;

		memcpy(send_rsp, rsp_to_remote.rsp_msg, send_sz);
		rsp_to_remote.valid_rsp = true;
		printf("Sending......\n");
		printf("%s\n", send_rsp);
	} else {
		rsp_to_remote.valid_rsp = false;
		printf("Received empty message\n");
	}
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
	uint32_t next_report_interval = 0;
	uint32_t next_wakeup_interval = 0;	/* Interval value in ms */
	uint32_t wake_up_interval = 15000;	/* Interval value in ms */
	uint32_t slept_till = 0;
	rsp_to_remote.valid_rsp = false;
	char dev_id[DEV_ID_LEN];

	sys_init();
	dbg_module_init();


	dbg_printf("Initializing communications module\n");
	ASSERT(cc_init(ctrl_cb));

	oem_init();

	if (!utils_get_device_id(dev_id, DEV_ID_LEN, NET_INTFC))
		dbg_printf("Can not retrieve device mac address\n");
	else
		dbg_printf("Device IMEI or mac address is: %s\n", dev_id);

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
		if (rsp_to_remote.valid_rsp)
			send_msg();
		next_report_interval = send_status_msg();

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
		if (wake_up_interval > next_report_interval) {
			wake_up_interval = next_report_interval;
			dbg_printf("Reporting required in %"
				   PRIu32" sec.\n", wake_up_interval / 1000);
		}
		dbg_printf("Powering down for %"PRIu32" seconds\n\n",
				wake_up_interval / 1000);
		slept_till = sys_sleep_ms(wake_up_interval);
		slept_till = wake_up_interval - slept_till;
		dbg_printf("Slept for %"PRIu32" seconds\n\n", slept_till / 1000);
	}
	return 0;
}
