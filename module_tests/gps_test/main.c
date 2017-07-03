/* Copyright (c) 2017 Verizon. All rights reserved */

/*
 * Minimal test application program. It initializes the GPS module, receive the 
 * location information from it and sends this to the cloud.  
 * To keep the test program simple, error handling is kept to a minimum.
 * Wherever possible, the program halts with an ASSERT in case the API fails.
 */

#include <string.h>
#include "sys.h"
#include "gpio_hal.h"
#include "dbg.h"
#include "gps_hal.h"

#include "cloud_comm.h"
#include "cc_basic_service.h"
#include "cc_control_service.h"
#include "dev_creds.h"
#include "ts_sdk_board_config.h" 

#define SEND_TIMEOUT_MS		2000
#define IDLE_CHARS		5

static volatile bool received_response;
static volatile bool received_gps_text;

struct parsed_nmea_t parsedNMEA;

typedef struct {
        uint8_t sz;             /* Number of bytes contained in the data buffer */
        uint8_t *bytes;         /* Pointer to the data buffer */
} array_t;

#define SEND_DATA_SZ	22
#define RECV_DATA_SZ	CC_MAX_RECV_BUF_SZ
#define RESEND_CALIB	0x42		/* Resend calibration command */
#define NONE		(-1)		/* Represents 'Wake up interval not set' */

CC_SEND_BUFFER(send_buffer, SEND_DATA_SZ);
CC_RECV_BUFFER(recv_buffer, RECV_DATA_SZ);

#if defined (OTT_PROTOCOL)
#define REMOTE_HOST	"iwk.ott.thingspace.verizon.com:443"
#elif defined (SMSNAS_PROTOCOL)
#define REMOTE_HOST	"+12345678912"
#else
#error "define valid protocol options from OTT_PROTOCOL or SMSNAS_PROTOCOL"
#endif

static bool resend_calibration;		/* Set if RESEND command was received */

/* Long sleep time in milliseconds */
#define LONG_SLEEP_INT_MS	180000

/* Status report interval in milliseconds */
#define STATUS_REPORT_INT_MS	60000

/* Last status message sent timestamp */
uint64_t last_st_ts = 0;

static void receive_completed(cc_buffer_desc *buf)
{
	cc_data_sz sz = cc_get_receive_data_len(buf, CC_SERVICE_BASIC);
	const uint8_t *recvd = cc_get_recv_buffer_ptr(buf, CC_SERVICE_BASIC);

	dbg_printf("\t\t\t");
	dbg_printf("Received Message: \n");
	for (cc_data_sz i = 0; i < sz; i++)
		dbg_printf("\t\t\t\t[Byte %u]: 0x%x, ", i, recvd[i]);
	dbg_printf("\n");

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


int insert_location_into_buffer(uint8_t *buffer, struct parsed_nmea_t *nmea)
{
	union cvt {
		float val;
		unsigned char b[4];
	} x;

	int noOfBytes = 10;
	x.val = nmea->latitudeDegrees;
	memcpy(buffer, x.b, noOfBytes/2);

	x.val = nmea->longitudeDegrees;
	memcpy(buffer+4, x.b, noOfBytes/2);

	memcpy(buffer+8, &nmea->fixquality, 1);
	memcpy(buffer+9, &nmea->satellites, 1);
	dbg_printf("GPS Latitude: %f Longitude: %f\n",
	nmea->latitudeDegrees,nmea->longitudeDegrees);
	return noOfBytes;
}

static uint32_t read_and_send_gps_data(uint64_t cur_ts)
{
	if (last_st_ts != 0) {
		if ((cur_ts - last_st_ts) < STATUS_REPORT_INT_MS) {
			return last_st_ts + STATUS_REPORT_INT_MS - cur_ts;
		}
	}

	array_t data;
	data.bytes = cc_get_send_buffer_ptr(&send_buffer, CC_SERVICE_BASIC);
	data.sz = 0;
	if (gps_new_NMEA_received()) {
		gps_parse_nmea(gps_last_NMEA(), &parsedNMEA);
		uint8_t *buffer = data.bytes + data.sz;
		int ret = insert_location_into_buffer(buffer, &parsedNMEA);
		data.sz = data.sz + ret;
		// add in 3 to the count to compensate for the header. This is temporary,
		// till the FW team figures things out.
		//data.sz += 3;
	}
	dbg_printf("Sending: %d bytes to cloud\n",data.sz);
	ASSERT(cc_send_svc_msg_to_cloud(&send_buffer, data.sz,
		CC_SERVICE_BASIC) == CC_SEND_SUCCESS);

	last_st_ts = cur_ts;
	return STATUS_REPORT_INT_MS;
}

int main(int argc, char *argv[])
{
	uint32_t next_wakeup_interval = 0;	/* Interval value in ms */
	uint32_t wake_up_interval = 15000;	/* Interval value in ms */
	uint32_t next_report_interval = 0;	/* Interval value in ms */
	uint32_t slept_till = 0;
        
	sys_init();
	dbg_module_init();
	dbg_printf("Begin :\n");

	pin_name_t pin_name = PC12;
	gpio_config_t my_led;
        my_led.dir = OUTPUT;
        my_led.pull_mode = PP_NO_PULL; 
        my_led.speed = SPEED_VERY_HIGH; 
        gpio_init(pin_name, &my_led);

	dbg_printf("Initializing GPS module\n");
	ASSERT(gps_module_init(IDLE_CHARS) == true);
	
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
	sys_sleep_ms(2000);

	dbg_printf("Make a receive buffer available\n");
	ASSERT(cc_set_recv_buffer(&recv_buffer) == CC_RECV_SUCCESS);

	dbg_printf("Sending \"restarted\" message\n");
	ASSERT(cc_ctrl_resend_init_config() == CC_SEND_SUCCESS);

	/* Set onboard LEDS */
	gpio_write(pin_name, PIN_LOW);

	while (1) {
	next_report_interval = read_and_send_gps_data(sys_get_tick_ms());

	next_wakeup_interval = cc_service_send_receive(sys_get_tick_ms());
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
	gpio_write(pin_name, PIN_HIGH);
	}
	return 0;
}
