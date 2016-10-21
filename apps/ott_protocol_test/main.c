/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include <inttypes.h>
#include "dbg.h"
#include "ott_protocol.h"
#include "platform.h"

#if 0
#define SERVER_PORT "4433"
#define SERVER_NAME "localhost"
#else
/* Temporary - a site that happens to speak our single crypto suite. */
#define SERVER_PORT "443"
#define SERVER_NAME "www.tapr.org"
#endif

static msg_t msg;
#define RECV_TIMEOUT_MS		7000

/* Wait for a response from the cloud. If none, return. */
#define EXPECT_RESPONSE_FROM_CLOUD(_msg) \
do { \
	platform_delay(RECV_TIMEOUT_MS); \
	if (ott_retrieve_msg(&(_msg)) != OTT_OK) { \
		dbg_printf("\tERR: No response from cloud, closing connection.\n"); \
		ott_send_ctrl_msg(CF_NACK | CF_QUIT); \
		ott_close_connection(); \
		return; \
	} \
} while(0)

/* Check for an ACK, failing which return. */
#define EXPECT_ACK_FROM_CLOUD(_msg, _flags) \
do { \
	OTT_LOAD_FLAGS(*(uint8_t *)&(_msg), (_flags)); \
	if (!OTT_FLAG_IS_SET((_flags), CF_ACK)) { \
		dbg_printf("\tERR: Cloud did not ACK previous message.\n"); \
		ott_send_ctrl_msg(CF_NACK | CF_QUIT); \
		ott_close_connection(); \
		return; \
	} \
} while(0)

void test_scenario_1(void)
{
	/* Scenario 1: Device has status to report, Cloud has nothing pending */

	c_flags_t c_flags;
	/* 1 - Initiate the TCP/TLS session */
	dbg_printf("Initiating a secure connection to the cloud\n");
	if (ott_initiate_connection(SERVER_NAME, SERVER_PORT) != OTT_OK) {
		dbg_printf("\tCould not connect to the cloud.\n");
		return;
	}

	/* 2 & 3 - Send the version byte and authenticate the device with the cloud. */
	uint8_t d_ID[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	uint8_t d_sec[] = {0, 1, 2, 3, 4, 5, 6};
	dbg_printf("Sending version byte and authentication message\n");
	ASSERT(ott_send_auth_to_cloud(CF_PENDING, d_ID, sizeof(d_sec), d_sec)
			== OTT_OK);

	/* 4 - Wait for a timeout before checking for a message from the cloud. */
	EXPECT_RESPONSE_FROM_CLOUD(msg);

	/* Make sure the previous message was ACKed. */
	EXPECT_ACK_FROM_CLOUD(msg, c_flags);
	dbg_printf("\tAuthenticated with the cloud.\n");

	/* 5 - Send the status of the sensors attached to the device. */
	uint8_t status[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	dbg_printf("Sending device status to the cloud\n");
	ASSERT(ott_send_status_to_cloud(CF_NONE, sizeof(status), status) == OTT_OK);

	/* 6 - Wait for a timeout before checking for a message from the cloud. */
	EXPECT_RESPONSE_FROM_CLOUD(msg);

	/* Make sure the previous message was ACKed. */
	EXPECT_ACK_FROM_CLOUD(msg, c_flags);
	dbg_printf("\tDevice status delivered to the cloud.\n");

	/* 7 - In this scenario, the cloud sets the QUIT flag. */
	OTT_LOAD_FLAGS(*(uint8_t *)&msg, c_flags);
	ASSERT(OTT_FLAG_IS_SET(c_flags, CF_QUIT));
	dbg_printf("Closing secure connection.\n");
	ASSERT(ott_close_connection() == OTT_OK);
}

void test_scenario_2(void)
{
	/* Scenario 2: Device has status to report and Cloud has pending updates */

	c_flags_t c_flags;
	/* 1 - Initiate the TCP/TLS session */
	dbg_printf("Initiating a secure connection to the cloud\n");
	if (ott_initiate_connection(SERVER_NAME, SERVER_PORT) != OTT_OK) {
		dbg_printf("\tCould not connect to the cloud.\n");
		return;
	}

	/* 2 & 3 - Send the version byte and authenticate the device with the cloud. */
	uint8_t d_ID[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	uint8_t d_sec[] = {0, 1, 2, 3, 4, 5, 6};
	dbg_printf("Sending version byte and authentication message\n");
	ASSERT(ott_send_auth_to_cloud(CF_PENDING, d_ID, sizeof(d_sec), d_sec)
			== OTT_OK);

	/* 4 - Wait for a timeout before checking for a message from the cloud. */
	EXPECT_RESPONSE_FROM_CLOUD(msg);

	/* Make sure the previous message was ACKed. */
	EXPECT_ACK_FROM_CLOUD(msg, c_flags);
	dbg_printf("\tAuthenticated with the cloud.\n");

	/* Read the update message from the cloud. */
	dbg_printf("Reading update sent from the cloud.\n");
	ASSERT(OTT_FLAG_IS_SET(c_flags, CF_PENDING));
	dbg_printf("\tSize : %d", msg.data.array.sz);
	dbg_printf("\tData :\n");
	for (uint8_t i = 0; i < msg.data.array.sz; i++)
		dbg_printf("\t0x%2X\n", i);

	/* 5 - Send the status of the sensors attached to the device. */
	uint8_t status[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
	dbg_printf("Sending device status to the cloud\n");
	ASSERT(ott_send_status_to_cloud(CF_ACK, sizeof(status), status) == OTT_OK);

	/* 6 - Wait for a timeout before checking for a message from the cloud. */
	EXPECT_RESPONSE_FROM_CLOUD(msg);

	/* Make sure the previous message was ACKed. */
	EXPECT_ACK_FROM_CLOUD(msg, c_flags);
	dbg_printf("\tDevice status delivered to the cloud.\n");

	/* Read the interval from the message */
	dbg_printf("Polling interval set: %"PRIu32" sec.\n", msg.data.interval);

	/* 7 & 8- Acknowledge the last message sent by the cloud and end comm. */
	dbg_printf("Closing secure connection.\n");
	ASSERT(ott_send_ctrl_msg(CF_ACK | CF_QUIT) == OTT_OK);
	ASSERT(ott_close_connection() == OTT_OK);
}

void test_scenario_3(void)
{
	/* Scenario 3 : Device has no status to report, Cloud has pending updates */

	c_flags_t c_flags;
	/* 1 - Initiate the TCP/TLS session */
	dbg_printf("Initiating a secure connection to the cloud\n");
	if (ott_initiate_connection(SERVER_NAME, SERVER_PORT) != OTT_OK) {
		dbg_printf("\tCould not connect to the cloud.\n");
		return;
	}

	/* 2 & 3 - Send the version byte and authenticate the device with the cloud. */
	uint8_t d_ID[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
	uint8_t d_sec[] = {0, 1, 2, 3, 4, 5, 6};
	dbg_printf("Sending version byte and authentication message\n");
	ASSERT(ott_send_auth_to_cloud(CF_NONE, d_ID, sizeof(d_sec), d_sec)
			== OTT_OK);

	/* 4 - Wait for a timeout before checking for a message from the cloud. */
	EXPECT_RESPONSE_FROM_CLOUD(msg);

	/* Make sure the previous message was ACKed. */
	EXPECT_ACK_FROM_CLOUD(msg, c_flags);
	dbg_printf("\tAuthenticated with the cloud.\n");

	/* Read the update message from the cloud. */
	dbg_printf("Reading update sent from the cloud.\n");
	ASSERT(OTT_FLAG_IS_SET(c_flags, CF_PENDING));
	dbg_printf("\tSize : %d", msg.data.array.sz);
	dbg_printf("\tData :\n");
	for (uint8_t i = 0; i < msg.data.array.sz; i++)
		dbg_printf("\t0x%2X\n", i);

	/* 5 - Acknowledge the last message received and end communication */
	dbg_printf("Closing secure connection.\n");
	ASSERT(ott_send_ctrl_msg(CF_ACK | CF_QUIT) == OTT_OK);
	ASSERT(ott_close_connection() == OTT_OK);
}

#define DELAY_MS	20000
int main(int argc, char *argv[])
{
	platform_init();

	dbg_module_init();
	ASSERT(ott_protocol_init() == OTT_OK);

	dbg_printf("Begin:\n");
	while (1) {
		dbg_printf("Test scenario 1\n");
		test_scenario_1();
		platform_delay(DELAY_MS);
		dbg_printf("Test scenario 2\n");
		test_scenario_2();
		platform_delay(DELAY_MS);
		dbg_printf("Test scenario 3\n");
		test_scenario_3();
		platform_delay(DELAY_MS);
	}
	return 0;
}
