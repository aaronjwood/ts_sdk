/* Copyright(C) 2016 Verizon. All rights reserved. */

/*
 * Example firmware that reads the BMP-180 pressure / temperature sensor and
 * reports the reading back to the cloud using the OTT protocol.
 */

#include "platform.h"
#include "cloud_comm.h"
#include "dbg.h"
#include "dev_creds.h"
#include "sensor_interface.h"

#define SENSOR_MASK	0x0001
#define SEND_DATA_SZ	OTT_DATA_SZ
#define RECV_DATA_SZ	OTT_DATA_SZ
#define RESEND_CALIB	0x42		/* Resend calibration command */

CC_SEND_BUFFER(send_buffer, SEND_DATA_SZ);
CC_RECV_BUFFER(recv_buffer, RECV_DATA_SZ);

#define SERVER_NAME	"test1.ott.uu.net"
#define SERVER_PORT	"443"
#define NUM_STATUSES	((uint8_t)4)

static bool resend_calibration;

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
		dbg_printf("\t\t\tSleep interval (secs): %"PRIu32"\n", sl_sec);
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

int main(int argc, char *argv[])
{
	array_t data;				/* Holds data from sensor */
	int32_t next_wakeup_interval = -1;	/* Interval value in ms */
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
	ASSERT(si_init(SENSOR_MASK) == SENSOR_MASK);

	data.bytes = cc_get_send_buffer_ptr(&send_buffer);
	ASSERT(si_read_calib(SENSOR_MASK, SEND_DATA_SZ, &data) == SENSOR_MASK);
	dbg_printf("\tSending out calibration data\n");
	ASSERT(cc_send_bytes_to_cloud(&send_buffer, data.sz, send_cb)
			== CC_SEND_SUCCESS);

	while (1) {
		dbg_printf("Reading sensor data\n");
		data.bytes = cc_get_send_buffer_ptr(&send_buffer);
		ASSERT(si_read_data(SENSOR_MASK, SEND_DATA_SZ, &data) == SENSOR_MASK);
		dbg_printf("\tSending out status message : %d bytes\n", data.sz);
		ASSERT(cc_send_bytes_to_cloud(&send_buffer,
					data.sz,
					send_cb) == CC_SEND_SUCCESS);
		if (resend_calibration) {
			resend_calibration = false;
			dbg_printf("\tResending calibration data\n");
			ASSERT(si_read_calib(SENSOR_MASK, SEND_DATA_SZ, &data)
					== SENSOR_MASK);
			ASSERT(cc_send_bytes_to_cloud(&send_buffer,
						data.sz,
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
		ASSERT(si_sleep(SENSOR_MASK) == SENSOR_MASK);
		platform_delay(wake_up_interval);
		ASSERT(si_wakeup(SENSOR_MASK) == SENSOR_MASK);
	}
	return 0;
}
