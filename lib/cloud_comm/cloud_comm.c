/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>

#include "platform.h"
#include "cloud_comm.h"
#include "ott_protocol.h"
#include "dbg.h"

#define RECV_TIMEOUT_MS		5000
#define MAX_HOST_LEN		25
#define MAX_PORT_LEN		5

static struct {				/* Store authentication data */
	uint8_t dev_ID[OTT_UUID_SZ];	/* 16 byte Device ID */
	array_t *d_sec;			/* Device secret */
} auth;

static struct {
	bool send_in_progress;		/* Set if a message is currently being sent */
	const cc_buffer_desc *buf;	/* Outgoing data buffer */
	cc_callback_rtn cb;		/* "Send" callback */
	bool send_ack;			/* Set if the current message should ACK
					 * the last received message.
					 */
} outgoing_conn;

static struct {
	bool recv_in_progress;		/* Set if an async receive was scheduled */
	cc_buffer_desc *buf;		/* Incoming data buffer */
	cc_callback_rtn cb;		/* "Receive" callback */
} incoming_conn;

static struct {
	bool auth_done;			/* Auth was sent for this session */
	bool keep_alive;		/* Cloud has a pending message / Device
					 * has to respond to the last message
					 * received, keep the connection alive.
					 */
	char host[MAX_HOST_LEN + 1];	/* Store the host name */
	char port[MAX_PORT_LEN + 1];	/* Store the host port */
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

/* Functions that reset the internal state and close the connection */
static inline void reset_conn_and_session_states(void)
{
	outgoing_conn.send_in_progress = false;
	incoming_conn.recv_in_progress = false;
	outgoing_conn.send_ack = false;
	session.auth_done = false;
	session.keep_alive = false;
}

static inline void reset_callbacks_and_buffers(void)
{
	outgoing_conn.buf = NULL;
	outgoing_conn.cb = NULL;
	incoming_conn.buf = NULL;
	incoming_conn.cb = NULL;
}

static inline void on_recv_quit(void)
{
	ott_close_connection();
	reset_conn_and_session_states();
}

static inline void initiate_quit(bool send_nack)
{
	if (!session.auth_done)
		return;
	c_flags_t c_flags = send_nack ? (CF_NACK | CF_QUIT) : CF_QUIT;
	ott_send_ctrl_msg(c_flags);
	ott_close_connection();
	reset_conn_and_session_states();
}

static inline void init_state(void)
{
	reset_callbacks_and_buffers();
	reset_conn_and_session_states();
}

bool cc_init(const uint8_t *d_ID, uint16_t d_sec_sz, const uint8_t *d_sec)
{
	/* Check for invalid parameters */
	if (d_ID == NULL || d_sec == NULL || d_sec_sz == 0 ||
			(d_sec_sz + OTT_UUID_SZ > OTT_DATA_SZ))
		return false;

	/* Store the auth information for establishing connection to the cloud */
	memcpy(auth.dev_ID, d_ID, OTT_UUID_SZ);
	auth.d_sec = malloc(sizeof(array_t) + d_sec_sz * sizeof (uint8_t));
	auth.d_sec->sz = d_sec_sz;
	memcpy(auth.d_sec->bytes, d_sec, auth.d_sec->sz);

	if (ott_protocol_init() != OTT_OK)
		return false;

	init_state();

	session.host[0] = 0x00;
	session.port[0] = 0x00;

	return true;
}

uint8_t *cc_get_send_buffer_ptr(cc_buffer_desc *buf)
{
	return (buf) ? buf->buf_ptr : NULL;
}

const uint8_t *cc_get_recv_buffer_ptr(const cc_buffer_desc *buf)
{
	/* Check for non-NULL buffer pointers */
	if (!buf || !buf->buf_ptr)
		return NULL;

	/*
	 * Depending on the type of message in the buffer, return a pointer
	 * to the "interval" field of the message's data or the "bytes" field.
	 */
	m_type_t m_type;
	const msg_t *ptr_to_msg = (const msg_t *)(buf->buf_ptr);
	OTT_LOAD_MTYPE(ptr_to_msg->cmd_byte, m_type);

	switch (m_type) {
	case MT_UPDATE:
		return (const uint8_t *)&(ptr_to_msg->data.array.bytes);
	case MT_CMD_PI:
	case MT_CMD_SL:
		return (const uint8_t *)&(ptr_to_msg->data.interval);
	default:
		/* XXX: Unlikely because of checks in ott_retrieve_msg() */
		return NULL;
	}
}

cc_data_sz cc_get_receive_data_len(const cc_buffer_desc *buf)
{
	/* Check for non-NULL buffer pointers */
	if (!buf || !buf->buf_ptr)
		return 0;

	/*
	 * The message can have one of two lengths depending on the type of
	 * message received: length of the "interval" field or length of the
	 * "bytes" field.
	 */
	m_type_t m_type;
	msg_t *ptr_to_msg = (msg_t *)(buf->buf_ptr);
	OTT_LOAD_MTYPE(ptr_to_msg->cmd_byte, m_type);

	switch (m_type) {
	case MT_UPDATE:
		return ptr_to_msg->data.array.sz;
	case MT_CMD_PI:
	case MT_CMD_SL:
		return sizeof(ptr_to_msg->data.interval);
	default:
		/* XXX: Unlikely because of checks in ott_retrieve_msg() */
		return 0;
	}
}

bool cc_set_remote_host(const char *host, const char *port)
{
	if (!host || !port)
		return false;

	size_t hlen = strlen(host);
	if (hlen > MAX_HOST_LEN || hlen == 0)
		return false;

	size_t plen = strlen(port);
	if (plen > MAX_PORT_LEN || plen == 0)
		return false;

	strncpy(session.host, host, hlen);
	session.host[MAX_HOST_LEN] = 0x00;
	strncpy(session.port, port, plen);
	session.port[MAX_PORT_LEN] = 0x00;

	return true;
}

void cc_ack_bytes(void)
{
	outgoing_conn.send_ack = true;
}

void cc_nak_bytes(void)
{
	initiate_quit(true);
}

/*
 * Process a received response. Most of the actual processing is done through the
 * user provided callbacks this function invokes. In addition, it sets some
 * internal flags to decide the state of the session in the future. Calling the
 * send callback is optional and is decided through "invoke_send_cb".
 */
static void process_recvd_msg(msg_t *msg_ptr, bool invoke_send_cb)
{
	incoming_conn.recv_in_progress = false;

	c_flags_t c_flags;
	m_type_t m_type;

	OTT_LOAD_FLAGS(msg_ptr->cmd_byte, c_flags);
	OTT_LOAD_MTYPE(msg_ptr->cmd_byte, m_type);

	/* Keep the session alive if the cloud has more messages to send */
	session.keep_alive = OTT_FLAG_IS_SET(c_flags, CF_PENDING);

	if (OTT_FLAG_IS_SET(c_flags, CF_ACK)) {
		cc_event evt = CC_STS_RCV_UPD;
		/* Messages with a body need to be ACKed / NACKed in the future */
		session.keep_alive = true;
		if (m_type == MT_NONE)
			session.keep_alive = false;
		else if (m_type == MT_UPDATE)
			evt = CC_STS_RCV_UPD;
		else if (m_type == MT_CMD_SL)
			evt = CC_STS_RCV_CMD_SL;

		/* Call the callbacks with the type of message received */
		if (invoke_send_cb)
			INVOKE_SEND_CALLBACK(outgoing_conn.buf, CC_STS_ACK);
		if (m_type != MT_NONE)
			INVOKE_RECV_CALLBACK(incoming_conn.buf, evt);
	} else if (OTT_FLAG_IS_SET(c_flags, CF_NACK)) {
		if (invoke_send_cb)
			INVOKE_SEND_CALLBACK(outgoing_conn.buf, CC_STS_NACK);
	}

	if (OTT_FLAG_IS_SET(c_flags, CF_QUIT))
		on_recv_quit();
}

/*
 * Attempt to receive a response within a timeout. Once a response is received,
 * process it. This helper function must be called everytime the device sends
 * a message to the cloud and expects a response in return.
 * "invoke_send_cb" decides if the send callback should be invoked on a valid
 * response. When expecting a response for an auth message (or any internal
 * message), this should be false.
 * On successfully receiving a valid response, this function will return "true",
 * otherwise "false".
 */
static bool recv_response_within_timeout(uint32_t timeout, bool invoke_send_cb)
{
	/* The "send" is said to be in progress until a valid response arrives
	 * or until timeout, whichever happens first.
	 */
	outgoing_conn.send_in_progress = true;

	uint32_t start = platform_get_tick_ms();
	uint32_t end = start;
	do {
		msg_t *msg_ptr = (msg_t *)incoming_conn.buf->buf_ptr;
		ott_status s = ott_retrieve_msg(msg_ptr, incoming_conn.buf->bufsz);

		if (s == OTT_INV_PARAM || s == OTT_ERROR) {
			outgoing_conn.send_in_progress = false;
			return false;
		}
		if (s == OTT_NO_MSG) {
			end = platform_get_tick_ms();
			continue;
		}
		if (s == OTT_OK) {
			process_recvd_msg(msg_ptr, invoke_send_cb);
			break;
		}
	} while(end - start < timeout);

	outgoing_conn.send_in_progress = false;

	if (end - start >= timeout)
		return false;

	return true;
}

static bool establish_session(const char *host, const char *port, bool polling)
{
	/* Check for invalid parameters */
	if (host == NULL || port == NULL || incoming_conn.buf == NULL ||
			incoming_conn.buf->buf_ptr == NULL ||
			!incoming_conn.recv_in_progress)
		return false;

	if (ott_initiate_connection(host, port) != OTT_OK)
		return false;

	/* Send the authentication message to the cloud. If this is a call to
	 * simply poll the cloud for possible messages, do not set the PENDING
	 * flag.
	 */
	c_flags_t c_flags = polling ? CF_NONE : CF_PENDING;
	if (ott_send_auth_to_cloud(c_flags, auth.dev_ID,
				auth.d_sec->sz, auth.d_sec->bytes) != OTT_OK) {
		initiate_quit(false);
		return false;
	}

	if (!recv_response_within_timeout(RECV_TIMEOUT_MS, false)) {
		initiate_quit(true);
		return false;
	}

	session.auth_done = true;
	return true;
}

static inline void close_session(void)
{
	if (!session.auth_done)
		return;

	initiate_quit(false);
	reset_callbacks_and_buffers();
}

cc_send_result cc_send_bytes_to_cloud(const cc_buffer_desc *buf, cc_data_sz sz,
		cc_callback_rtn cb)
{
	if (outgoing_conn.send_in_progress)
		return CC_SEND_BUSY;

	if (!buf || !buf->buf_ptr || sz == 0)
		return CC_SEND_FAILED;

	if (strlen(session.host) == 0 || strlen(session.port) == 0)
		return CC_SEND_FAILED;

	/*
	 * If a session hasn't been established, initiate a connection. This
	 * leads to the device being authenticated.
	 */
	if (!session.auth_done)
		if (!establish_session(session.host, session.port, false))
			return CC_SEND_FAILED;

	/*
	 * If the user ACKed, set the flag in this message. Also, make sure the
	 * PENDING flag is always set so that the device has full control on
	 * when to disconnect.
	 */
	c_flags_t c_flags = outgoing_conn.send_ack ? (CF_PENDING | CF_ACK) :
		CF_PENDING;
	if (ott_send_status_to_cloud(c_flags, sz,
				(const uint8_t *)buf->buf_ptr) != OTT_OK) {
		initiate_quit(false);
		return CC_SEND_FAILED;
	}

	outgoing_conn.buf = buf;
	outgoing_conn.send_ack = false;

	if (!recv_response_within_timeout(RECV_TIMEOUT_MS, true)) {
		initiate_quit(true);
		return CC_SEND_FAILED;
	}

	return CC_SEND_SUCCESS;
}

cc_recv_result cc_recv_bytes_from_cloud(cc_buffer_desc *buf, cc_callback_rtn cb)
{
	/* Check for invalid parameters */
	if (!buf || !buf->buf_ptr)
		return CC_RECV_FAILED;

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
