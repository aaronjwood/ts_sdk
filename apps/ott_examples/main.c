/* Copyright(C) 2016 Verizon. All rights reserved. */

/*
 * Example firmware that reads the sensors and reports the reading(s) back to
 * the cloud using the OTT protocol. It also reports back calibration data on
 * receiving a command (RESEND_CALIB).
 * Data from each sensor is sent in a separate message to the cloud.
 */

#include "platform.h"
#include "cloud_comm.h"
#include "dbg.h"
#include "dev_creds.h"
#include "sensor_interface.h"

#define SEND_DATA_SZ	22
#define RECV_DATA_SZ	CC_MIN_RECV_BUF_SZ
#define RESEND_CALIB	0x42		/* Resend calibration command */
#define NONE		(-1)		/* Represents 'Wake up interval not set' */

CC_SEND_BUFFER(send_buffer, SEND_DATA_SZ);
CC_RECV_BUFFER(recv_buffer, RECV_DATA_SZ);

#define SERVER_NAME	"test1.ott.uu.net"
#define SERVER_PORT	"443"

static bool resend_calibration;		/* Set if RESEND command was received */

/* Receive callback */
static void recv_cb(const cc_buffer_desc *buf, cc_event event)
{
	dbg_printf("\t\t[RECV CB] Received a message:\n");
	if (event == CC_STS_RCV_UPD) {
		cc_data_sz sz = cc_get_receive_data_len(buf);
		const uint8_t *recvd = cc_get_recv_buffer_ptr(buf);
		if (recvd[0] == RESEND_CALIB && sz == 1) {
			resend_calibration = true;
			dbg_printf("\t\t[RECV CB] Will send an ACK in response\n");
			cc_ack_bytes();
		} else {
			resend_calibration = false;
			dbg_printf("\t\t[RECV CB] NACKing an invalid message\n");
			cc_nak_bytes();
		}
	} else if (event == CC_STS_RCV_CMD_SL) {
		uint32_t sl_sec = cc_get_sleep_interval(buf);
		dbg_printf("\t\t\tSleep interval (secs) : %"PRIu32"\n", sl_sec);
		dbg_printf("\t\t[RECV CB] Will send an ACK in response\n");
		cc_ack_bytes();
	}

	/* Reschedule a receive */
	cc_recv_result s = cc_recv_bytes_from_cloud(&recv_buffer, recv_cb);
	ASSERT(s == CC_RECV_SUCCESS || s == CC_RECV_BUSY);
}

/* Send callback */
static void send_cb(const cc_buffer_desc *buf, cc_event event)
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

static void send_all_calibration_data(void)
{
	array_t data;
	data.bytes = cc_get_send_buffer_ptr(&send_buffer);
	ASSERT(si_read_calib(0, SEND_DATA_SZ, &data));
	dbg_printf("\tCalibration table : %d bytes\n", data.sz);
	ASSERT(cc_send_bytes_to_cloud(&send_buffer,
				data.sz,
				send_cb) == CC_SEND_SUCCESS);
}

static void read_and_send_all_sensor_data(void)
{
	array_t data;
	data.bytes = cc_get_send_buffer_ptr(&send_buffer);
	for (uint8_t i = 0; i < si_get_num_sensors(); i++) {
		ASSERT(si_read_data(i, SEND_DATA_SZ, &data));
		dbg_printf("\tSensor [%d], ", i);
		dbg_printf("sending out status message : %d bytes\n", data.sz);
		ASSERT(cc_send_bytes_to_cloud(&send_buffer,
					data.sz,
					send_cb) == CC_SEND_SUCCESS);
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
	ASSERT(cc_init(d_ID, sizeof(d_sec), d_sec));

	dbg_printf("Setting remote host and port\n");
	ASSERT(cc_set_remote_host(SERVER_NAME, SERVER_PORT));

	dbg_printf("Scheduling a receive\n");
	ASSERT(cc_recv_bytes_from_cloud(&recv_buffer, recv_cb) == CC_RECV_SUCCESS);

	dbg_printf("Sending \"restarted\" message\n");
	ASSERT(cc_resend_init_config(send_cb) == CC_SEND_SUCCESS);

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
