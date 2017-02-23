/* Copyright(C) 2016,2017 Verizon. All rights reserved. */

/*
 * Example firmware that reads the sensors and reports the reading(s) back to
 * the cloud using the OTT protocol. It also reports back calibration data on
 * receiving a command (RESEND_CALIB).
 * Data from each sensor is sent in a separate message to the cloud.
 */

#include "platform.h"
#include "cloud_comm.h"
#include "cc_basic_service.h"
#include "cc_control_service.h"
#include "dbg.h"
#include "dev_creds.h"
#include "sensor_interface.h"

#define SEND_DATA_SZ	22
#define RECV_DATA_SZ	CC_MIN_RECV_BUF_SZ
#define RESEND_CALIB	0x42		/* Resend calibration command */
#define NONE		(-1)		/* Represents 'Wake up interval not set' */

CC_SEND_BUFFER(send_buffer, CC_MAX_SEND_BUF_SZ);
CC_RECV_BUFFER(recv_buffer, CC_MAX_RECV_BUF_SZ);

#define SERVER_NAME	"iwk.ott.thingspace.verizon.com"
#define SERVER_PORT	"443"

static bool resend_calibration;		/* Set if RESEND command was received */

static void receive_completed(cc_buffer_desc *buf)
{
	cc_data_sz sz = cc_get_receive_data_len(buf);
	const uint8_t *recvd = cc_get_recv_buffer_ptr(buf);

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
	data.bytes = cc_get_send_buffer_ptr(&send_buffer);
	ASSERT(si_read_calib(0, SEND_DATA_SZ, &data));
	dbg_printf("\tCalibration table : %d bytes\n", data.sz);
	ASSERT(cc_send_svc_msg_to_cloud(&send_buffer, data.sz,
					CC_SERVICE_BASIC) == CC_SEND_SUCCESS);
}

static void read_and_send_all_sensor_data(void)
{
	array_t data;
	data.bytes = cc_get_send_buffer_ptr(&send_buffer);
	for (uint8_t i = 0; i < si_get_num_sensors(); i++) {
		ASSERT(si_read_data(i, SEND_DATA_SZ, &data));
		dbg_printf("\tSensor [%d], ", i);
		dbg_printf("sending out status message : %d bytes\n", data.sz);
		ASSERT(cc_send_svc_msg_to_cloud(&send_buffer, data.sz,
						CC_SERVICE_BASIC)
		       == CC_SEND_SUCCESS);
	}
}

int main(int argc, char *argv[])
{
	int32_t next_wakeup_interval = NONE;	/* Interval value in ms */
	uint32_t cur_ts;			/* Current timestamp in ms */
	int32_t wake_up_interval = 15000;	/* Interval value in ms */

	platform_init();
	dbg_module_init();
	dbg_printf("Begin:\n");

	dbg_printf("Initializing communications module\n");
	ASSERT(cc_init(ctrl_cb));

	dbg_printf("Register to use the Basic service\n");
	ASSERT(cc_register_service(&cc_basic_service_descriptor,
				   basic_service_cb));

	dbg_printf("Setting remote host and port\n");
	ASSERT(cc_set_destination(SERVER_NAME ":" SERVER_PORT));
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
		dbg_printf("Reading sensor data\n");
		read_and_send_all_sensor_data();
		if (resend_calibration) {
			resend_calibration = false;
			dbg_printf("\tResending calibration data\n");
			send_all_calibration_data();
		}

		cur_ts = platform_get_tick_ms();
		next_wakeup_interval = cc_service_send_receive(cur_ts);
		if (next_wakeup_interval != NONE) {
			if (wake_up_interval != next_wakeup_interval)
				dbg_printf("New wakeup time received\n");
			dbg_printf("Relative wakeup time set to +%"PRIi32" sec\n",
					next_wakeup_interval / 1000);
			wake_up_interval = next_wakeup_interval;
		}

		dbg_printf("Powering down for %"PRIi32" seconds\n\n",
				wake_up_interval / 1000);
		ASSERT(si_sleep());
		platform_delay(wake_up_interval);
		ASSERT(si_wakeup());
	}
	return 0;
}
