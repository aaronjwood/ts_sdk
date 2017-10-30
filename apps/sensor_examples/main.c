/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

/*
 * Example firmware that reads the sensors and reports the reading(s) back to
 * the cloud using the OTT protocol. It also reports back calibration data on
 * receiving a command (RESEND_CALIB).
 * Data from each sensor is sent in a separate message to the cloud.
 * To keep the test program simple, error handling is kept to a minimum.
 * Wherever possible, the program halts with an ASSERT in case the API fails.
 */

#include <string.h>
#include "sys.h"
#include "cloud_comm.h"
#include "cc_basic_service.h"
#include "cc_control_service.h"
#include "dbg.h"
#include "protocol_init.h"
#include "sensor_interface.h"
#include "hmc5883l_interpret.h"

#if defined(FREE_RTOS)
#include "cmsis_os.h"

enum {
	THREAD_READER = 0,
	THREAD_SENDER
};

osThreadId reader_thread1_handle, sender_thread2_handle;
static void reader_thread1(void const *argument);
static void sender_thread2(void const *argument);
#endif

uint64_t last_st_ts = 0;
#define RESEND_CALIB   0x42

CC_SEND_BUFFER(send_buffer, CC_MAX_SEND_BUF_SZ);
CC_SEND_BUFFER(cal_send_buffer, CC_MAX_SEND_BUF_SZ);
CC_RECV_BUFFER(recv_buffer, CC_MAX_RECV_BUF_SZ);

static bool resend_calibration;		/* Set if RESEND command was received */

/* Number of times to retry sending in case of failure */
#define MAX_RETRIES	((uint8_t)3)

/* Arbitrary long sleep time in milliseconds */
#define LONG_SLEEP_INT_MS	180000

/* status report interval in milliseconds */
#define STATUS_REPORT_INT_MS	15000
/* last status message sent timestamp */

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
#if defined (OTT_PROTOCOL) || defined (SMSNAS_PROTOCOL)
		res = cc_send_svc_msg_to_cloud(b, s, id, NULL);
#elif defined (MQTT_PROTOCOL)
		res = cc_send_status_msg_to_cloud(b, s);
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

#define MAX_NUM_SENSORS 5
#define MAX_DATA_SZ 30

static array_t data[MAX_NUM_SENSORS];
static array_t caldata;

/* Array for sensor data buffer */
static uint8_t rbytes[MAX_NUM_SENSORS][MAX_DATA_SZ];
/* Array for calibration buffer */
static uint8_t calbytes[MAX_DATA_SZ];

static void send_all_calibration_data(void)
{
	array_t data;
	data.bytes = cc_get_send_buffer_ptr(&send_buffer, CC_SERVICE_BASIC);
	memcpy(data.bytes, calbytes, caldata.sz);
	dbg_printf("\tCalibration table : %d bytes\n", caldata.sz);
	send_with_retry(&send_buffer, caldata.sz, CC_SERVICE_BASIC);
}

static uint32_t send_all_sensor_data(uint64_t cur_ts)
{
	if (last_st_ts != 0) {
		if ((cur_ts - last_st_ts) < STATUS_REPORT_INT_MS) {
			dbg_printf("Returning without sending sensor data\n");
			return last_st_ts + STATUS_REPORT_INT_MS - cur_ts;
		}
	}
	dbg_printf("Sending sensor data\n");
	for (uint8_t i = 0; i < si_get_num_sensors(); i++) {
		data[i].bytes = cc_get_send_buffer_ptr(&send_buffer,
				CC_SERVICE_BASIC);
		memcpy(data[i].bytes, rbytes[i], data[i].sz);
		dbg_printf("\tSensor [%d], ", i);
		dbg_printf("sending out status message : %d bytes\n",
			data[i].sz);
#ifdef MQTT_PROTOCOL
		cc_data_sz sz = 0;
		dbg_printf("\tSensor HMC" );
		send_json_payload(&send_buffer, &sz);
		data[i].sz = sz;
#endif
		send_with_retry(&send_buffer, data[i].sz, CC_SERVICE_BASIC);
	}

	if (resend_calibration) {
		resend_calibration = false;
		dbg_printf("\tResending calibration data\n");
		send_all_calibration_data();
	}
	last_st_ts = cur_ts;

	return STATUS_REPORT_INT_MS;
}

static uint32_t read_all_sensor_data()
{
	dbg_printf("Reading sensor data\n");
	for (uint8_t i = 0; i < si_get_num_sensors(); i++) {
		data[i].bytes = rbytes[i];
		ASSERT(si_read_data(i, SEND_DATA_SZ, &data[i]));
		dbg_printf("\tSensor [%d], ", i);
	}
	dbg_printf("\n\tReading Sensor data Completed\n");

	dbg_printf("Reading Calibration data now\n");

	/* Reading Sensor Calibration data */
	caldata.bytes = calbytes;
	ASSERT(si_read_calib(0, SEND_DATA_SZ, &caldata));

	return STATUS_REPORT_INT_MS;
}

static void communication_init(void)
{
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

	dbg_printf("Make a receive buffer available\n");
	ASSERT(cc_set_recv_buffer(&recv_buffer) == CC_RECV_SUCCESS);

	dbg_printf("Sending \"restarted\" message\n");
	ASSERT(cc_ctrl_resend_init_config() == CC_SEND_SUCCESS);

	dbg_printf("Initializing the sensors\n");
	ASSERT(si_init());

}

static void receive_and_wait_for_reporting(uint32_t next_report_interval)
{
	uint32_t next_wakeup_interval = 0;	/* Interval value in ms */
	uint32_t slept_till = 0;
	uint32_t wake_up_interval = 15000;	/* Interval value in ms */

	next_wakeup_interval = cc_service_send_receive(
						sys_get_tick_ms());
	if (next_wakeup_interval == 0) {
		wake_up_interval = LONG_SLEEP_INT_MS;
		dbg_printf("Protocol does not required to be called"
			",sleeping for %"PRIu32" sec.\n",
			wake_up_interval / 1000);
	} else {
		dbg_printf("Protocol requests wakeup in %"PRIu32
		"sec.\n", next_wakeup_interval / 1000);
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
	dbg_printf("Slept for %"PRIu32" seconds\n\n",
		slept_till / 1000);
	ASSERT(si_wakeup());
}

#if defined(FREE_RTOS)
static void reader_thread1(void const *argument)
{
	(void) argument;

	while (1) {
		/* Resume Thread 2 */
		osThreadResume(sender_thread2_handle);
		/* Suspend Thread 1 : current thread */
		osThreadSuspend(reader_thread1_handle);

		read_all_sensor_data();
	}
}

static void sender_thread2(void const *argument)
{
	uint32_t next_report_interval = 0;	/* Interval in ms */
	(void) argument;

	communication_init();
	while (1) {
		/* Resume Thread 1 */
		osThreadResume(reader_thread1_handle);
		/* Suspend Thread2 : current thread */
		osThreadSuspend(sender_thread2_handle);

		next_report_interval = send_all_sensor_data(
							sys_get_tick_ms());

		receive_and_wait_for_reporting(next_report_interval);
	}
}

static void create_threads(void)
{
	dbg_printf("Creating the sensors reader thread\n");
	/* Thread 1 definition */
	osThreadDef(THREAD_READER, reader_thread1, osPriorityNormal, 0,
		configMINIMAL_STACK_SIZE);

	dbg_printf("Creating the sensors sender thread\n");
	/* Thread 2 definition */
	osThreadDef(THREAD_SENDER, sender_thread2, osPriorityNormal, 0,
		configMINIMAL_STACK_SIZE);

	/* Start thread 1 */
	reader_thread1_handle = osThreadCreate(osThread(THREAD_READER), NULL);

	/* Start thread 2 */
	sender_thread2_handle = osThreadCreate(osThread(THREAD_SENDER), NULL);

	/* Set thread 2 in suspend state */
	osThreadSuspend(sender_thread2_handle);

	/* Start scheduler */
	osKernelStart();

	while (1) {
		/* We should never get here as control is now taken by the
		 * scheduler
		 */
	}
}
#endif

int main(void)
{
	sys_init();
	dbg_module_init();
	dbg_printf("Begin:\n");

#if defined(FREE_RTOS)
	create_threads();
#else
	uint32_t next_report_interval = 0;	/* Interval in ms */

	communication_init();
	while (1) {
		read_all_sensor_data();
		next_report_interval = send_all_sensor_data(
							sys_get_tick_ms());
		receive_and_wait_for_reporting(next_report_interval);
	}
#endif

	return 0;
}
