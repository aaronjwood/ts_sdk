/* Copyright (c) 2017 Verizon. All rights reserved */

/*
 * Minimal test application program. It initializes the GPS module and
 * BME280 sensors. It receives pressure, temprature, humidity from
 * BME280 sensor and location information from gps.
 * It packages and sends these information to cloud.
 * To keep the test program simple, error handling is kept to a minimum.
 * Wherever possible, the program halts with an ASSERT in case the API fails.
 */

#include <string.h>
#include "sys.h"
#include "dbg.h"
#include "gpio_hal.h"
#include "gps_hal.h"
#include "sensor_interface.h"

#include "cloud_comm.h"
#include "cc_basic_service.h"
#include "cc_control_service.h"
#include "protocol_init.h"

struct parsed_nmea_t parsedNMEA;

#define RECV_DATA_SZ    CC_MAX_RECV_BUF_SZ
#define RESEND_CALIB    0x42	/* Resend calibration command */
#define NONE            (-1)	/* Represents 'Wake up interval not set' */

CC_SEND_BUFFER(send_buffer, CC_MAX_SEND_BUF_SZ);
CC_RECV_BUFFER(recv_buffer, RECV_DATA_SZ);

static bool resend_calibration;         /* Set if RESEND command was received */

/* Number of times to retry sending in case of failure */
#define MAX_RETRIES     ((uint8_t)3)

/* Arbitrary long sleep time in milliseconds */
#define LONG_SLEEP_INT_MS       180000

/* status report interval in milliseconds */
#define STATUS_REPORT_INT_MS    60000
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

static void send_with_retry(cc_buffer_desc *b, cc_data_sz s, cc_service_id id)
{
	uint8_t retries = 0;
	cc_send_result res;
	while (retries < MAX_RETRIES) {
	#if defined (MQTT_PROTOCOL)
		res = cc_send_status_msg_to_cloud(b, s);
	#else
                res = cc_send_svc_msg_to_cloud(b, s, id, NULL);
	#endif
		if (res == CC_SEND_SUCCESS)
			break;
		retries++;
		dbg_printf("\t%s: send attempt %d out of %d failed\n", __func__,
		retries, MAX_RETRIES);
	}
	if (res != CC_SEND_SUCCESS)
		dbg_printf("\t%s: Failed to send message.\n", __func__);
}


static int insert_location_into_buffer(uint8_t *buffer,
				 struct parsed_nmea_t *nmea)
{
	union cvt {
		float val;
		unsigned char b[4];
	} x;

	int noOfBytes = 10;
	x.val = nmea->latitude_degrees;
	memcpy(buffer, x.b, 4);

	x.val = nmea->longitude_degrees;
	memcpy(buffer+4, x.b, 4);

	memcpy(buffer+8, &nmea->fix_quality, 1);
	memcpy(buffer+9, &nmea->satellites, 1);
	return noOfBytes;
}

static uint32_t read_and_send_data(uint64_t cur_ts)
{

	if (last_st_ts != 0) {
		if ((cur_ts - last_st_ts) < STATUS_REPORT_INT_MS)
			return last_st_ts + STATUS_REPORT_INT_MS - cur_ts;
	}

	array_t data;
	data.bytes = cc_get_send_buffer_ptr(&send_buffer, CC_SERVICE_BASIC);
	si_read_data(&data);

	if (gps_receive(&parsedNMEA)) {
		uint8_t *buffer = data.bytes + data.sz;
		int ret = insert_location_into_buffer(buffer, &parsedNMEA);
		data.sz = data.sz + ret;
	}

	dbg_printf("Sending: %d bytes to cloud\n", data.sz);
	send_with_retry(&send_buffer, data.sz, CC_SERVICE_BASIC);

	last_st_ts = cur_ts;
	return STATUS_REPORT_INT_MS;
}

int main()
{

	uint32_t next_wakeup_interval = 0;      /* Interval value in ms */
	uint32_t wake_up_interval = 15000;      /* Interval value in ms */
	uint32_t next_report_interval = 0;      /* Interval value in ms */
	uint32_t slept_till = 0;

	pin_name_t pin_name = PC12;
	gpio_config_t my_led;
	my_led.dir = OUTPUT;
	my_led.pull_mode = PP_NO_PULL;
	my_led.speed = SPEED_VERY_HIGH;
	gpio_init(pin_name, &my_led);

	sys_init();
	dbg_module_init();
	dbg_printf("Begin :\n");

	dbg_printf("Initializing BME280 sensor\n\r");
	ASSERT(si_init());

	dbg_printf("Initializing GPS module\n");
	ASSERT(gps_module_init() == true);

	dbg_printf("Initializing communications module\n");
	ASSERT(cc_init(ctrl_cb));

	dbg_printf("Register to use the Basic service\n");
	ASSERT(cc_register_service(&cc_basic_service_descriptor,
				basic_service_cb));

	dbg_printf("Setting remote destination\n");
	ASSERT(cc_set_destination(REMOTE_HOST));

	dbg_printf("Setting device authentiation credentials\n");
	ASSERT(cc_set_own_auth_credentials(cl_cred, CL_CRED_SZ,
				cl_sec_key, CL_SEC_KEY_SZ));

	dbg_printf("Setting remote side authentiation credentials\n");
	ASSERT(cc_set_remote_credentials(cacert, CA_CRED_SZ));
	sys_sleep_ms(2000);

	dbg_printf("Make a receive buffer available\n");
	ASSERT(cc_set_recv_buffer(&recv_buffer) == CC_RECV_SUCCESS);

	dbg_printf("Sending \"restarted\" message\n");
	ASSERT(cc_ctrl_resend_init_config() == CC_SEND_SUCCESS);

	/* Set onboard LEDS */
	gpio_write(pin_name, PIN_LOW);

	while (1) {
		next_report_interval = read_and_send_data(sys_get_tick_ms());
		next_wakeup_interval = cc_service_send_receive(sys_get_tick_ms());
		if (next_wakeup_interval == 0) {
			wake_up_interval = LONG_SLEEP_INT_MS;
			dbg_printf("Protocol does not required to be called"
			",sleeping for %"PRIu32" sec.\n",
			wake_up_interval / 1000);
		} else {
			dbg_printf("Protocol requests wakeup in %"
			PRIu32" sec.\n", next_wakeup_interval / 1000);
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
		dbg_printf("Slept for %"PRIu32" seconds\n\n",
				 slept_till / 1000);
		gpio_write(pin_name, PIN_HIGH);
	}
	return 0;
}
