/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "dbg.h"
#include "ott_protocol.h"
#include "platform.h"

/*
 * The dev-net server used for testing:
 * #define SERVER_NAME "testott.vzbi.com"
 * #define SERVER_PORT "443"
 */
#define SERVER_NAME "test1.ott.uu.net"
#define SERVER_PORT "443"

#define RECV_TIMEOUT_MS		10000	/* Receive timeout in ms */
#define BUF_SZ			512	/* Max receive buffer size in bytes */
#define INIT_PI_MS		5000	/* Initial polling interval in ms */
#define NUM_STATUSES		3	/* Number of statuses to send */
#define INTERVAL_MULT		1000	/* Interval value multiplier */

/* Wait for a response from the cloud. On timeout, return with an error. */
#define EXPECT_RESPONSE_FROM_CLOUD(_msg_ptr, _sz) \
do { \
	ott_status s = ott_retrieve_msg(_msg_ptr, _sz); \
	if (s != OTT_OK && s != OTT_NO_MSG) { \
		dbg_printf("\tERR: No response from cloud, closing connection.\n"); \
		ott_send_ctrl_msg(CF_QUIT); \
		ott_close_connection(); \
		return false; \
	} else { \
		break; \
	} \
} while(platform_get_tick_ms() - start_time < RECV_TIMEOUT_MS)

/* Check for an ACK, failing which return with an error. */
#define EXPECT_ACK_FROM_CLOUD(_msg_ptr) \
do { \
	c_flags_t c_flags; \
	m_type_t m_type; \
	OTT_LOAD_FLAGS(_msg_ptr->cmd_byte, c_flags); \
	OTT_LOAD_MTYPE(_msg_ptr->cmd_byte, m_type); \
	if (!OTT_FLAG_IS_SET(c_flags, CF_ACK)) { \
		dbg_printf("\tERR: Cloud did not ACK previous message.\n"); \
		interpret_type_flags(m_type, c_flags); \
		ott_send_ctrl_msg(CF_QUIT); \
		ott_close_connection(); \
		return false; \
	} \
} while(0)

/* Select which device credentials to use */
#define DEVICE_NUMBER		0

/* Device ID and device secret */
#if DEVICE_NUMBER == 0
/* For reference: QR Code = "pU5sUnOF1FmWcV9y" */
static const uint8_t d_ID[] = {
	0xd1, 0x56, 0xe2, 0x79, 0xff, 0xda, 0x4c, 0x03,
	0xa7, 0x76, 0x0d, 0x18, 0xf2, 0x2f, 0x43, 0x22
};

static const uint8_t d_sec[] = {
	0x7c, 0x1b, 0x90, 0xfa, 0x84, 0x09, 0x4c, 0xfe,
	0xaa, 0x11, 0x1f, 0x03, 0x42, 0x0f, 0xc4, 0x7f,
	0x27, 0xc9, 0x29, 0x03, 0xb1, 0xc4, 0xce, 0x2a,
	0xc3, 0xc9, 0x4d, 0x8b, 0x9a, 0x53, 0x6e, 0xed
};
#endif

static volatile bool run;

#ifdef BUILD_TARGET_OSX
extern void ott_protocol_deinit(void);
void SIGINT_Handler(int dummy)
{
	/* Ensure clean exit from the super loop on <Ctrl+C> on OS X */
	run = false;
}
#define EVAL_RETURN(x)	do { if (!(x)) goto cleanup; } while(0)
#else
#define EVAL_RETURN(x)	do { if (!(x)) ASSERT(0); } while(0)
#endif

static uint32_t p_int_ms;	/* Polling interval in milliseconds */
static uint32_t sl_int_ms;	/* Sleep interval in milliseconds */

/* Interpret the type and flag fields of the command byte */
static void interpret_type_flags(m_type_t m_type, c_flags_t c_flags)
{
	dbg_printf("\tMessage type: ");
	if (m_type == MT_NONE)
		dbg_printf("MT_NONE\n");
	else if (m_type == MT_AUTH)
		dbg_printf("MT_AUTH\n");
	else if (m_type == MT_STATUS)
		dbg_printf("MT_STATUS\n");
	else if (m_type == MT_UPDATE)
		dbg_printf("MT_UPDATE\n");
	else if (m_type == MT_CMD_PI)
		dbg_printf("MT_CMD_PI\n");
	else if (m_type == MT_CMD_SL)
		dbg_printf("MT_CMD_SL\n");
	else
		dbg_printf("Invalid message type\n");

	dbg_printf("\tFlags set: ");
	if (OTT_FLAG_IS_SET(c_flags, CF_NONE))
		dbg_printf("CF_NONE ");
	if (OTT_FLAG_IS_SET(c_flags, CF_NACK))
		dbg_printf("CF_NACK ");
	if (OTT_FLAG_IS_SET(c_flags, CF_ACK))
		dbg_printf("CF_ACK ");
	if (OTT_FLAG_IS_SET(c_flags, CF_PENDING))
		dbg_printf("CF_PENDING ");
	if (OTT_FLAG_IS_SET(c_flags, CF_QUIT))
		dbg_printf("CF_QUIT ");
	dbg_printf("\n");
}

/* Interpret the received message. If the message is empty, return false */
static bool interpret_message(msg_t *msg)
{
	if (!msg)
		return false;
	m_type_t m_type;
	c_flags_t c_flags;
	OTT_LOAD_MTYPE(msg->cmd_byte, m_type);
	OTT_LOAD_FLAGS(msg->cmd_byte, c_flags);
	interpret_type_flags(m_type, c_flags);
	switch (m_type) {
	case MT_UPDATE:
		dbg_printf("\tSize : %"PRIu16"\n", msg->data.array.sz);
		dbg_printf("\tData :\n");
		for (uint8_t i = 0; i < msg->data.array.sz; i++)
			dbg_printf("\t0x%02x\n", i);
		return true;
	case MT_CMD_SL:
		sl_int_ms = msg->data.interval;
		dbg_printf("\tSleep interval (secs): %"PRIu32"\n", sl_int_ms);
		sl_int_ms *= INTERVAL_MULT;
		return true;
	case MT_CMD_PI:
		p_int_ms = msg->data.interval;
		dbg_printf("\tPolling interval (secs): %"PRIu32"\n", p_int_ms);
		p_int_ms *= INTERVAL_MULT;
		return true;
	case MT_NONE:
		/* Fall through */
	default:
		return false;
	}
}

/* Attempt to establish a secure connection to the cloud */
static bool authenticate_device(msg_t *msg, bool *ack_pending)
{
	if (!msg || !ack_pending)
		return false;

	memset(msg, 0, BUF_SZ);

	dbg_printf("Initiating a secure connection to the cloud\n");
	if (ott_initiate_connection(SERVER_NAME, SERVER_PORT) != OTT_OK) {
		dbg_printf("\tCould not connect to the cloud.\n");
		return false;
	}

	dbg_printf("Sending version byte and authentication message\n");
	ASSERT(ott_send_auth_to_cloud(CF_PENDING, d_ID, sizeof(d_sec), d_sec)
			== OTT_OK);

	/* Timeout if there is no response from the cloud. */
	uint32_t start_time = platform_get_tick_ms();
	EXPECT_RESPONSE_FROM_CLOUD(msg, BUF_SZ);

	/* Make sure the previous message was ACKed. */
	EXPECT_ACK_FROM_CLOUD(msg);
	dbg_printf("\tAuthenticated with the cloud.\n");

	/* Print any message data. If it is present, it needs to be ACKed */
	*ack_pending = interpret_message(msg);

	return true;
}

/* Exchange messages with the cloud */
static bool transact_msgs(msg_t *msg, uint8_t num_statuses, bool ack_pending)
{
	if (num_statuses == 0 || !msg)
		return false;

	bool keep_alive;	/* Set if the connection should be kept alive */

	c_flags_t c_flags_recv;
	c_flags_t c_flags_send = ack_pending ? CF_ACK : CF_NONE;
	uint8_t status[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	for (int i = 0; i < num_statuses; i++) {
		dbg_printf("Sending device status (%d) to the cloud\n", i + 1);
		ASSERT(ott_send_status_to_cloud(c_flags_send | CF_PENDING,
						sizeof(status),
						status) == OTT_OK);

		/* Timeout if there is no response from the cloud. */
		uint32_t start_time = platform_get_tick_ms();
		EXPECT_RESPONSE_FROM_CLOUD(msg, BUF_SZ);

		/* Make sure the previous message was ACKed. */
		EXPECT_ACK_FROM_CLOUD(msg);
		OTT_LOAD_FLAGS(msg->cmd_byte, c_flags_recv);
		dbg_printf("\tDevice status delivered to the cloud.\n");

		/* If this message has data, it needs to be ACKed (or NACKed) */
		bool msg_body_present = interpret_message(msg);
		c_flags_send = msg_body_present ? CF_ACK : CF_NONE;

		/*
		 * Check if the cloud has any additional messages to send or if
		 * the current message needs to be responded to.
		 */
		keep_alive = OTT_FLAG_IS_SET(c_flags_recv, CF_PENDING) ||
			msg_body_present;
	}

	/* Retrieve additional messages the cloud needs to send and ACK any
	 * previous data messages. */
	while (keep_alive) {
		ASSERT(ott_send_ctrl_msg(c_flags_send) == OTT_OK);

		/* Timeout if there is no response from the cloud. */
		uint32_t start_time = platform_get_tick_ms();
		EXPECT_RESPONSE_FROM_CLOUD(msg, BUF_SZ);

		/* Check for the pending flag */
		OTT_LOAD_FLAGS(msg->cmd_byte, c_flags_recv);

		/* Print any data attached to this message */
		bool msg_body_present = interpret_message(msg);
		c_flags_send = msg_body_present ? CF_ACK : CF_NONE;

		/*
		 * Check if the cloud has any additional messages to send or if
		 * the current message needs to be responded to.
		 */
		keep_alive = OTT_FLAG_IS_SET(c_flags_recv, CF_PENDING) ||
			msg_body_present;
	}

	return true;
}

static bool close_connection(msg_t *msg)
{
	if (msg == NULL)
		return false;

	/*
	 * If the cloud initiated the QUIT, end the connection. Else, send a
	 * control message with the QUIT flag and then end the connection.
	 */
	dbg_printf("Closing connection.\n");
	c_flags_t c_flags;
	OTT_LOAD_FLAGS(msg->cmd_byte, c_flags);
	if (!OTT_FLAG_IS_SET(c_flags, CF_QUIT))
		ASSERT(ott_send_ctrl_msg(CF_QUIT) == OTT_OK);
	ASSERT(ott_close_connection() == OTT_OK);
	return true;
}

int main(int argc, char *argv[])
{
#ifdef BUILD_TARGET_OSX
	signal(SIGINT, SIGINT_Handler);
#endif
	platform_init();

	dbg_module_init();
	dbg_printf("Begin:\n");
	dbg_printf("Initializing protocol and underlying comm hardware..");
	ASSERT(ott_protocol_init() == OTT_OK);
	dbg_printf("Done.\n");
	msg_t *msg = malloc(BUF_SZ);

#ifdef BUILD_TARGET_OSX
	dbg_printf("Press <Ctrl+C> to exit\n");
#endif

	run = true;
	p_int_ms = INIT_PI_MS;
	bool ack_pending = false;

	dbg_printf("Server name: %s\n", SERVER_NAME);
	dbg_printf("Server port: %s\n", SERVER_PORT);

	/*
	 * This test authenticates the device with the cloud, receives any
	 * commands in return, sends a fixed number of status messages back
	 * to the cloud (while receiving any pending commands / updates) and
	 * then closes the connection.
	 * Any received message is ACKed by default.
	 * In this test, the device always has data to send while polling.
	 */
	while (run) {
		EVAL_RETURN(authenticate_device(msg, &ack_pending));
		EVAL_RETURN(transact_msgs(msg, NUM_STATUSES, ack_pending));
		EVAL_RETURN(close_connection(msg));

		dbg_printf("\n");

		/* Wait until the next polling time */
		platform_delay(p_int_ms);
	}

#ifndef BUILD_TARGET_OSX
	while (1)
		;
#else
cleanup:
	free(msg);
	ott_protocol_deinit();
	dbg_printf("Closed connection and exiting test.\n");
#endif
	return 0;
}
