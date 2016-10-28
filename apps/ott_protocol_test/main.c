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
#define SERVER_PORT "443"
#define SERVER_NAME "test1.ott.uu.net"

#define RECV_TIMEOUT_MS		7000	/* Receive timeout in ms */
#define BUF_SZ			512	/* Receive buffer size in bytes */
#define INIT_PI_MS		5000	/* Initial polling interval in ms */
#define NUM_STATUSES		3	/* Number of statuses to send */
#define INTERVAL_MULT		1000	/* Interval multiplier */

/* Select which device credentials to use: 0 or 1 */
#define DEVICE_NUMBER		0

/* Wait for a response from the cloud. On timeout, return with an error. */
#define EXPECT_RESPONSE_FROM_CLOUD(_msg_ptr, _sz) \
do { \
	if (ott_retrieve_msg(_msg_ptr, _sz) != OTT_OK) { \
		dbg_printf("\tERR: No response from cloud, closing connection.\n"); \
		ott_send_ctrl_msg(CF_QUIT); \
		ott_close_connection(); \
		return false; \
	} else { \
		break; \
	} \
} while(platform_get_tick_ms() - start_time < RECV_TIMEOUT_MS)

/* Check for an ACK, failing which return with an error. */
#define EXPECT_ACK_FROM_CLOUD(_msg_ptr, _flags) \
do { \
	OTT_LOAD_FLAGS(*(uint8_t *)(_msg_ptr), (_flags)); \
	if (!OTT_FLAG_IS_SET((_flags), CF_ACK)) { \
		dbg_printf("\tERR: Cloud did not ACK previous message.\n"); \
		ott_send_ctrl_msg(CF_QUIT); \
		ott_close_connection(); \
		return false; \
	} \
} while(0)

/* Device ID and device secret */
#if DEVICE_NUMBER == 0
static const uint8_t d_ID[] = {
	0x92, 0x94, 0x78, 0xe7, 0x55, 0x1d, 0x46, 0xe7,
	0xb3, 0x76, 0x9c, 0xbc, 0xde, 0xb4, 0x8b, 0x7e
};

static const uint8_t d_sec[] = {
	0x53, 0x9a, 0x89, 0xa1, 0xb0, 0xac, 0xf4, 0xfd,
	0xd1, 0x32, 0xfe, 0x8e, 0x02, 0x57, 0x4e, 0xbf,
	0xfa, 0x3b, 0xa4, 0xb3, 0xd8, 0xdf, 0xd8, 0xe0,
	0x5e, 0x37, 0xab, 0x87, 0x64, 0x03, 0x5a, 0x3b
};
#elif DEVICE_NUMBER == 1
static const uint8_t d_ID[] = {
	0x29, 0x7f, 0xa6, 0x68, 0x86, 0x69, 0x4b, 0x30,
	0xb0, 0x3a, 0xe2, 0xb3, 0xf7, 0x08, 0x00, 0xfb
};

static const uint8_t d_sec[] = {
	0x2d, 0xe1, 0x3f, 0xfb, 0xba, 0x2e, 0x94, 0xc3,
	0xcd, 0x9a, 0xf2, 0xde, 0xc8, 0xff, 0xbc, 0xab,
	0x30, 0xf6, 0x09, 0xfc, 0x33, 0x35, 0x16, 0xb0,
	0x2d, 0xed, 0xf7, 0xea, 0xc3, 0x1c, 0x6b, 0x9f
};
#endif

static volatile bool run;

#ifdef BUILD_TARGET_OSX
extern void ott_protocol_deinit(void);
void SIGINT_Handler(int dummy)
{
	/* Ensure clean exit from the super loop on Ctrl+C on OS X */
	run = false;
}
#define EVAL_RETURN(x)	do { if (!(x)) goto cleanup; } while(0)
#else
#define EVAL_RETURN(x)	do { if (!(x)) ASSERT(0); } while(0)
#endif

static uint32_t p_int_ms;	/* Polling interval in milliseconds */
static uint32_t sl_int_ms;	/* Sleep interval in milliseconds */

/* Interpret the type and flag fields of the command byte */
static void interpret_type_flags(c_flags_t c_flags, m_type_t m_type)
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
	OTT_LOAD_MTYPE(*(uint8_t *)msg, m_type);
	OTT_LOAD_FLAGS(*(uint8_t *)msg, c_flags);
	interpret_type_flags(c_flags, m_type);
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
	if (!msg)
		return false;

	memset(msg, 0, BUF_SZ);

	c_flags_t c_flags;
	uint32_t start_time;

	dbg_printf("Initiating a secure connection to the cloud\n");
	if (ott_initiate_connection(SERVER_NAME, SERVER_PORT) != OTT_OK) {
		dbg_printf("\tCould not connect to the cloud.\n");
		return false;
	}

	dbg_printf("Sending version byte and authentication message\n");
	ASSERT(ott_send_auth_to_cloud(CF_PENDING, d_ID, sizeof(d_sec), d_sec)
			== OTT_OK);

	/* Timeout if there is no response from the cloud. */
	start_time = platform_get_tick_ms();
	EXPECT_RESPONSE_FROM_CLOUD(msg, BUF_SZ);

	/* Make sure the previous message was ACKed. */
	EXPECT_ACK_FROM_CLOUD(msg, c_flags);
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
	uint8_t start_time;
	uint8_t status[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

	for (int i = 0; i < num_statuses; i++) {
		dbg_printf("Sending device status (%d) to the cloud\n", i + 1);
		ASSERT(ott_send_status_to_cloud(c_flags_send | CF_PENDING,
						sizeof(status),
						status) == OTT_OK);

		/* Timeout if there is no response from the cloud. */
		start_time = platform_get_tick_ms();
		EXPECT_RESPONSE_FROM_CLOUD(msg, BUF_SZ);

		/* Make sure the previous message was ACKed. */
		EXPECT_ACK_FROM_CLOUD(msg, c_flags_recv);
		dbg_printf("\tDevice status delivered to the cloud.\n");

		/* If this message has data, it needs to be ACKed (or NACKed) */
		c_flags_send = interpret_message(msg) ? CF_ACK : CF_NONE;

		/* Check if the cloud has any additional messages to send */
		keep_alive = OTT_FLAG_IS_SET(c_flags_recv, CF_PENDING);
	}

	/* Retrieve additional messages the cloud needs to send and ACK any
	 * previous data messages. */
	while (keep_alive || !OTT_FLAG_IS_SET(c_flags_send, CF_NONE)) {
		ASSERT(ott_send_ctrl_msg(c_flags_send) == OTT_OK);

		/* Timeout if there is no response from the cloud. */
		start_time = platform_get_tick_ms();
		EXPECT_RESPONSE_FROM_CLOUD(msg, BUF_SZ);

		/* Check for the pending flag */
		OTT_LOAD_FLAGS(*(uint8_t *)msg, c_flags_recv);
		keep_alive = OTT_FLAG_IS_SET(c_flags_recv, CF_PENDING);

		/* Print any data attached to this message */
		c_flags_send = interpret_message(msg) ? CF_ACK : CF_NONE;
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
	OTT_LOAD_FLAGS(*(uint8_t *)msg, c_flags);
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
	ASSERT(ott_protocol_init() == OTT_OK);
	msg_t *msg = malloc(BUF_SZ);

	dbg_printf("Begin:\n");

	run = true;
	p_int_ms = INIT_PI_MS;
	uint32_t start_time_ms = platform_get_tick_ms();
	bool ack_pending = false;

	dbg_printf("Server name: %s\n", SERVER_NAME);
	dbg_printf("Server port: %s\n", SERVER_PORT);

	while (run) {
		EVAL_RETURN(authenticate_device(msg, &ack_pending));
		EVAL_RETURN(transact_msgs(msg, NUM_STATUSES, ack_pending));
		EVAL_RETURN(close_connection(msg));

		dbg_printf("\n");

		/* Wait until the next polling time */
		start_time_ms = platform_get_tick_ms();
		while (platform_get_tick_ms() - start_time_ms < p_int_ms)
			continue;
	}

#ifndef BUILD_TARGET_OSX
	while (1)
		;
#else
cleanup:
	free(msg);
	ott_protocol_deinit();
#endif
	return 0;
}
