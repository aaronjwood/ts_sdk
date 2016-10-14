/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>
#include <stm32f4xx_hal.h>

#include "cloud_comm.h"
#include "ott_protocol.h"
#include "dbg.h"

/* XXX: Replace with our server:port */
#define SERVER_NAME	"www.tapr.org"
#define SERVER_PORT	"443"
#define TCP_TIMEOUT_MS	5000

static struct {				/* Store authentication data */
	uint8_t dev_ID[OTT_UUID_SZ];	/* 16 byte Device ID */
	array_t *d_sec;			/* Device secret */
} auth_params;

static struct {
	bool send_in_progress;		/* Set if a message is currently sent */
	bool auth_done;			/* Auth was sent for this session */
	const msg_packet_t *msg;	/* Actual message */
} outgoing_conn;

static struct {
	bool recv_in_progress;		/* Set if an async receive was scheduled */
	bool msg_recvd;			/* Set if a complete message was received */
	msg_packet_t *msg;		/* Actual message */
} incoming_conn;

bool cc_init(const uint8_t *d_ID, uint16_t d_sec_sz, const uint8_t *d_sec)
{
	/* Check for valid parameters */
	if (d_ID == NULL || d_sec == NULL || d_sec_sz == 0 ||
			(d_sec_sz + OTT_UUID_SZ > OTT_DATA_SZ))
		return false;

	/* Store the auth information for establishing connection to the cloud */
	memcpy(auth_params.dev_ID, d_ID, OTT_UUID_SZ);
	auth_params.d_sec = malloc(sizeof(array_t) + d_sec_sz * sizeof (uint8_t));
	auth_params.d_sec->sz = d_sec_sz;
	memcpy(auth_params.d_sec->bytes, d_sec, auth_params.d_sec->sz);

	/* Initialize the OTT protocol API */
	if (ott_protocol_init() != OTT_OK)
		return false;

	/* Initialize the incoming and outgoing connection objects */
	outgoing_conn.send_in_progress = false;
	outgoing_conn.auth_done = false;
	outgoing_conn.msg = NULL;
	incoming_conn.recv_in_progress = false;
	incoming_conn.msg_recvd = false;
	incoming_conn.msg = NULL;

	return true;
}

uint8_t *cc_get_send_buffer_ptr(cc_buffer_desc *buf)
{
	return ((msg_packet_t *)(buf))->array.bytes;
}

uint8_t *cc_get_recv_buffer_ptr(cc_buffer_desc *buf)
{
	return ((msg_packet_t *)(buf))->array.bytes;
}

cc_data_sz cc_get_receive_data_len(cc_buffer_desc *buf)
{
	return ((msg_packet_t *)(buf))->array.sz;
}

bool cc_send_in_progress(void)
{
	return outgoing_conn.send_in_progress;
}

cc_send_result cc_send_bytes_to_cloud(const cc_buffer_desc *buf, cc_data_sz sz,
		cc_callback_rtn cb)
{
	/* Allow for only one send to be outstanding */
	if (outgoing_conn.send_in_progress)
		return CC_SEND_BUSY;

	outgoing_conn.send_in_progress = true;

	/* Initiate a connection to the cloud server */
	if (ott_initiate_connection(SERVER_NAME, SERVER_PORT) != OTT_OK)
		return CC_SEND_FAILED;

	/* Authenticate the device with the cloud */
	if (ott_send_auth_to_cloud(CF_PENDING,
				auth_params.dev_ID,
				auth_params.d_sec->sz,
				auth_params.d_sec->bytes) != OTT_OK) {
		ASSERT(ott_close_connection() == OTT_OK);
		return CC_SEND_FAILED;
	}

	/* Store a pointer to the data to be sent */
	outgoing_conn.msg = (const msg_packet_t *)buf->buf_ptr;

	return CC_SEND_SUCCESS;
}

cc_recv_result cc_recv_bytes_from_cloud(cc_buffer_desc *buf, cc_data_sz sz)
{
	/* Allow for only one receive to be outstanding */
	if (incoming_conn.recv_in_progress)
		return CC_RECV_BUSY;

	incoming_conn.recv_in_progress = true;

	memset(buf->buf_ptr, 0, buf->bufsz);
	return CC_RECV_SUCCESS;
}

int32_t cc_service_send_receive(int32_t current_timestamp)
{
#if 0
	uint32_t start = HAL_GetTick();
	bool timeout_error = true;
	while (HAL_GetTick() - start <= TCP_TIMEOUT_MS) {
		ott_status s = ott_retrieve_msg(&msg);
		if (s == OTT_OK) {
			timeout_error = false;
			msg_recvd = true;
			break;
		} else if (s == OTT_NO_MSG)
			continue;
		else
			break;
	}

	/* No response from the cloud: Attempt to send NACK+QUIT & close conn. */
	if (timeout_error) {
		ott_send_ctrl_msg(CF_NACK | CF_QUIT);
		ASSERT(ott_close_connection() == OTT_OK);
		return CC_SEND_FAILED;
	}

	/* Cloud replied with a NACK (+QUIT): Close connection */
	if (OTT_FLAG_IS_SET(msg.c_flags, CF_NACK)) {
		ASSERT(ott_close_connection() == OTT_OK);
		return CC_SEND_FAILED;
	}
#endif
	return 0;
}
