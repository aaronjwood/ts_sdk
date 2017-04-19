/* Copyright(C) 2016,2017 Verizon. All rights reserved. */

/*
 * Example firmware that reads the sensors and reports the reading(s) back to
 * the cloud using the OTT protocol. It also reports back calibration data on
 * receiving a command (RESEND_CALIB).
 * Data from each sensor is sent in a separate message to the cloud.
 */

#include "sys.h"
#include "cloud_comm.h"
#include "cc_basic_service.h"
#include "cc_control_service.h"
#include "dbg.h"
#include "dev_creds.h"
#include "sensor_interface.h"

#define SEND_DATA_SZ	22
#define RESEND_CALIB	0x42		/* Resend calibration command */

CC_SEND_BUFFER(send_buffer, CC_MAX_SEND_BUF_SZ);
CC_RECV_BUFFER(recv_buffer, CC_MAX_RECV_BUF_SZ);

#if defined (OTT_PROTOCOL)
#define REMOTE_HOST	"iwk.ott.thingspace.verizon.com:443"
#elif defined (SMSNAS_PROTOCOL)
#define REMOTE_HOST	"+12345678"
#else
#error "define valid protocol options from OTT_PROTOCOL or SMSNAS_PROTOCOL"
#endif

static bool resend_calibration;		/* Set if RESEND command was received */

/* Arbitrary long sleep time in milliseconds */
#define LONG_SLEEP_INT_MS	180000

/* status report interval in milliseconds */
#define STATUS_REPORT_INT_MS	15000
/* last status message sent timestamp */
uint64_t last_st_ts = 0;

static void receive_completed(cc_buffer_desc *buf)
{
	cc_data_sz sz = cc_get_receive_data_len(buf, CC_SERVICE_BASIC);
	const uint8_t *recvd = cc_get_recv_buffer_ptr(buf, CC_SERVICE_BASIC);

	if (recvd[0] == RESEND_CALIB && sz == 1) {
		resend_calibration = true;
		dbg_printf("\t\t\tWill send an ACK in response\n");
				cc_ack_msg();
	} else {
		resend_calibration = false;
		dbg_printf("\t\t\tNACKing an invalid message\n");
		cc_nak_msg();
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

static void send_all_calibration_data(void)
{
	array_t data;
	data.bytes = cc_get_send_buffer_ptr(&send_buffer, CC_SERVICE_BASIC);
	ASSERT(si_read_calib(0, SEND_DATA_SZ, &data));
	dbg_printf("\tCalibration table : %d bytes\n", data.sz);
	ASSERT(cc_send_svc_msg_to_cloud(&send_buffer, data.sz,
					CC_SERVICE_BASIC) == CC_SEND_SUCCESS);
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
		ASSERT(cc_send_svc_msg_to_cloud(&send_buffer, data.sz,
						CC_SERVICE_BASIC)
		       == CC_SEND_SUCCESS);
	}
	last_st_ts = cur_ts;
	return STATUS_REPORT_INT_MS;
}

int main(int argc, char *argv[])
{
	uint32_t next_wakeup_interval = 0;	/* Interval value in ms */
	uint32_t wake_up_interval = 15000;	/* Interval value in ms */
	uint32_t next_report_interval = 0;	/* Interval in ms */
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
	ASSERT(cc_set_auth_credentials(d_ID, sizeof(d_ID),
					d_sec, sizeof(d_sec)));

	dbg_printf("Make a receive buffer available\n");
	ASSERT(cc_set_recv_buffer(&recv_buffer) == CC_RECV_SUCCESS);

	dbg_printf("Sending \"restarted\" message\n");
	ASSERT(cc_ctrl_resend_init_config() == CC_SEND_SUCCESS);

	dbg_printf("Initializing the sensors\n");
	ASSERT(si_init());

	dbg_printf("Sending out calibration data\n");
	send_all_calibration_data();
	while (1) {
		next_report_interval = read_and_send_all_sensor_data(
							sys_get_tick_ms());
		if (resend_calibration) {
			resend_calibration = false;
			dbg_printf("\tResending calibration data\n");
			send_all_calibration_data();
		}

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

		if (wake_up_interval > next_report_interval) {
			wake_up_interval = next_report_interval;
			dbg_printf("Reporting required in %"
				   PRIu32" sec.\n", wake_up_interval / 1000);
		}
		dbg_printf("Powering down for %"PRIu32" seconds\n\n",
				wake_up_interval / 1000);
		ASSERT(si_sleep());
		slept_till = sys_sleep_ms(wake_up_interval);
		slept_till = wake_up_interval - slept_till;
		dbg_printf("Slept for %"PRIu32" seconds\n\n", slept_till / 1000);
		ASSERT(si_wakeup());
	}
	return 0;
}
