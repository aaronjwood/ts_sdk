/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>

#include "platform.h"
#include "cloud_comm.h"
#include "ott_protocol.h"
#include "dbg.h"

#define RECV_TIMEOUT_MS		5000

static struct {				/* Store authentication data */
	uint8_t dev_ID[OTT_UUID_SZ];	/* 16 byte Device ID */
	array_t *d_sec;			/* Device secret */
} auth;

static struct {
	bool send_in_progress;		/* Set if a message is currently sent */
	const cc_buffer_desc *buf;	/* Outgoing data buffer */
	cc_callback_rtn cb;		/* "Send" callback */
} outgoing_conn;

static struct {
	bool recv_in_progress;		/* Set if an async receive was scheduled */
	bool msg_recvd;			/* Set if a complete message was received */
	cc_buffer_desc *buf;		/* Incoming data buffer */
	cc_callback_rtn cb;		/* "Receive" callback */
} incoming_conn;

static struct {
	bool auth_done;			/* Auth was sent for this session */
	bool connection_established;	/* Set when a session is active */
} session;

#define INVOKE_SEND_CALLBACK(_buf, _evt) \
	do { \
		if (outgoing_conn.cb) \
			outgoing_conn.cb((cc_buffer_desc *)(_buf), (_evt)); \
	} while(0)

#define INVOKE_RECV_CALLBACK(_buf, _evt) \
	do { \
		if (incoming_conn.cb) \
			incoming_conn.cb((cc_buffer_desc *)(_buf), (_evt)); \
	} while(0)

#define CLEANLY_CLOSE_CONN() \
	do { \
		ott_send_ctrl_msg(CF_NACK | CF_QUIT); \
		ott_close_connection(); \
	} while(0)

bool cc_init(const uint8_t *d_ID, uint16_t d_sec_sz, const uint8_t *d_sec)
{
	/* Check for valid parameters */
	if (d_ID == NULL || d_sec == NULL || d_sec_sz == 0 ||
			(d_sec_sz + OTT_UUID_SZ > OTT_DATA_SZ))
		return false;

	/* Store the auth information for establishing connection to the cloud */
	memcpy(auth.dev_ID, d_ID, OTT_UUID_SZ);
	auth.d_sec = malloc(sizeof(array_t) + d_sec_sz * sizeof (uint8_t));
	auth.d_sec->sz = d_sec_sz;
	memcpy(auth.d_sec->bytes, d_sec, auth.d_sec->sz);

	/* Initialize the OTT protocol API */
	if (ott_protocol_init() != OTT_OK)
		return false;

	/* Initialize the incoming, outgoing and session objects */
	outgoing_conn.send_in_progress = false;
	outgoing_conn.buf = NULL;
	outgoing_conn.cb = NULL;

	incoming_conn.recv_in_progress = false;
	incoming_conn.msg_recvd = false;
	incoming_conn.buf = NULL;
	incoming_conn.cb = NULL;

	session.auth_done = false;
	session.connection_established = false;

	return true;
}

uint8_t *cc_get_send_buffer_ptr(cc_buffer_desc *buf)
{
	return (buf) ? buf->buf_ptr : NULL;
}

uint8_t *cc_get_recv_buffer_ptr(cc_buffer_desc *buf)
{
	/* Check for non-NULL buffer pointers */
	if (!buf || !buf->buf_ptr)
		return NULL;

	/*
	 * Depending on the type of message in the buffer, return a pointer
	 * to the "interval" field of the message's data or the "bytes" field.
	 */
	m_type_t m_type;
	OTT_LOAD_MTYPE(*(const uint8_t *)buf->buf_ptr, m_type);

	msg_t *ptr_to_msg = (msg_t *)(buf->buf_ptr);
	uint8_t *ptr_to_return = NULL;

	switch (m_type) {
	case MT_UPDATE:
		/* Return a pointer to the message's bytes */
		ptr_to_return = (uint8_t *)&(ptr_to_msg->data.array.bytes);
		return ptr_to_return;
	case MT_CMD_PI:
	case MT_CMD_SL:
		/* Return a pointer to the interval */
		ptr_to_return = (uint8_t *)&(ptr_to_msg->data.interval);
		return ptr_to_return;
	default:
		/* XXX: Unlikely because of checks in ott_retrieve_msg() */
		return NULL;
	}
}

cc_data_sz cc_get_receive_data_len(cc_buffer_desc *buf)
{
	/* Check for non-NULL buffer pointers */
	if (!buf || !buf->buf_ptr)
		return 0;

	/*
	 * The message can have one of two lengths depending on the type of
	 * message received:
	 * 1) sizeof(uint32_t) for command messages
	 * 2) sz field of the byte array received
	 */
	m_type_t m_type;
	OTT_LOAD_MTYPE(*(const uint8_t *)buf->buf_ptr, m_type);
	msg_t *ptr_to_msg = (msg_t *)(buf->buf_ptr);

	switch (m_type) {
	case MT_UPDATE:
		return ptr_to_msg->data.array.sz;
	case MT_CMD_PI:
	case MT_CMD_SL:
		return sizeof(uint32_t);
	default:
		/* XXX: Unlikely because of checks in ott_retrieve_msg() */
		return 0;
	}
}

/*
 * Attempt to receive from the cloud. If no message is currently available,
 * return CC_NO_MSG. If there was an error, return CC_RCV_ERR. If a message was
 * successfully received, return CC_MSG_AVAIL. If the remote peer disconnected,
 * return CC_QUIT.
 * When a message is available, the receive callback is always invoked.
 * This function takes a boolean parameter that decides if the send callback
 * should be invoked on receiving a response. The only case when this should
 * be false is when we are sending an internal message such as an auth.
 */
typedef enum {
	CC_NO_MSG,
	CC_RCV_ERR,
	CC_MSG_AVAIL,
	CC_QUIT
} resp_status;

static resp_status expect_response_from_cloud(bool invoke_send_callback)
{
	c_flags_t c_flags;
	m_type_t m_type;
	msg_t *msg_ptr = (msg_t *)incoming_conn.buf->buf_ptr;
	ott_status s = ott_retrieve_msg(msg_ptr, incoming_conn.buf->bufsz);

	if (s == OTT_INV_PARAM || s == OTT_ERROR) {
		CLEANLY_CLOSE_CONN();
		return CC_RCV_ERR;
	} else if (s == OTT_OK) {
		OTT_LOAD_FLAGS(*(uint8_t *)msg_ptr, c_flags);
		OTT_LOAD_MTYPE(*(uint8_t *)msg_ptr, m_type);

		if (OTT_FLAG_IS_SET(c_flags, CF_ACK)) {
			cc_event evt = CC_STS_RCV_CTRL;
			if (m_type == MT_NONE)
				evt = CC_STS_RCV_CTRL;
			else if (m_type == MT_UPDATE)
				evt = CC_STS_RCV_UPD;
			else if (m_type == MT_CMD_PI)
				evt = CC_STS_RCV_CMD_PI;
			else if (m_type == MT_CMD_SL)
				evt = CC_STS_RCV_CMD_SL;

			if (invoke_send_callback)
				INVOKE_SEND_CALLBACK(outgoing_conn.buf, CC_STS_ACK);
			INVOKE_RECV_CALLBACK(incoming_conn.buf, evt);
			return CC_MSG_AVAIL;
		} else if (OTT_FLAG_IS_SET(c_flags, CF_NACK)) {
			if (invoke_send_callback)
				INVOKE_SEND_CALLBACK(outgoing_conn.buf, CC_STS_NACK);
			CLEANLY_CLOSE_CONN();
			return CC_RCV_ERR;
		}

		if (OTT_FLAG_IS_SET(c_flags, CF_QUIT)) {
			ott_close_connection();
			return CC_QUIT;
		}

	} /* OTT_NO_MSG left unhandled */
	return CC_NO_MSG;
}

bool cc_establish_session(const char *host, const char *port)
{
	/* Check for invalid parameters */
	if (host == NULL || port == NULL ||
			incoming_conn.buf == NULL ||
			incoming_conn.buf->buf_ptr == NULL)
		return false;

	/* Prevent establishing multiple connections to the cloud services. */
	if (session.connection_established)
		return false;

	/* Initiate a connection to the cloud server */
	if (ott_initiate_connection(host, port) != OTT_OK)
		return false;

	/* Send the authentication message to the cloud. */
	if (ott_send_auth_to_cloud(CF_PENDING,
				auth.dev_ID,
				auth.d_sec->sz,
				auth.d_sec->bytes) != OTT_OK) {
		ott_close_connection();
		return false;
	}

	/* Verify that the cloud authenticated the device */
	uint32_t start = platform_get_tick_ms();
	uint32_t end;
	do {
		resp_status rsp = expect_response_from_cloud(false);
		if (rsp == CC_RCV_ERR)
			return false;
		else if (rsp == CC_MSG_AVAIL)
			break;
		else if (rsp == CC_NO_MSG)
			end = platform_get_tick_ms();
	} while (end - start < RECV_TIMEOUT_MS);

	if (end - start >= RECV_TIMEOUT_MS)	/* Timed out */
		return false;

	/* Successfully authenticated device with the cloud services */
	session.connection_established = true;
	session.auth_done = true;
	return true;
}

void cc_close_session(void)
{
	if (!session.connection_established)
		return;
	ott_send_ctrl_msg(CF_NACK | CF_QUIT);
	ott_close_connection();
	session.connection_established = false;
	session.auth_done = false;
}

cc_send_result cc_send_bytes_to_cloud(const cc_buffer_desc *buf, cc_data_sz sz,
		cc_callback_rtn cb)
{
	/* Ensure a connection has already been established */
	if (!session.connection_established || !session.auth_done)
		return CC_SEND_FAILED;

	/* Allow for only one send to be outstanding */
	if (outgoing_conn.send_in_progress)
		return CC_SEND_BUSY;

	outgoing_conn.send_in_progress = true;

	/* Store a pointer to the data to be sent */
	outgoing_conn.buf = buf;

	outgoing_conn.send_in_progress = false;

	return CC_SEND_SUCCESS;
}

cc_recv_result cc_recv_bytes_from_cloud(cc_buffer_desc *buf, cc_callback_rtn cb)
{
	/* Allow for only one receive to be outstanding */
	if (incoming_conn.recv_in_progress)
		return CC_RECV_BUSY;

	incoming_conn.recv_in_progress = true;
	incoming_conn.buf = buf;

	memset(buf->buf_ptr, 0, buf->bufsz + OTT_OVERHEAD_SZ);
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
