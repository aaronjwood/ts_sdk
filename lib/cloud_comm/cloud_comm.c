/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>

#include "platform.h"
#include "cloud_comm.h"
#include "ott_protocol.h"
#include "dbg.h"

#define RECV_TIMEOUT_MS		5000
#define MULT			1000
#define INIT_POLLLING_MS	((int32_t)15000)

static struct {				/* Store authentication data */
	uint8_t dev_ID[OTT_UUID_SZ];	/* 16 byte Device ID */
	array_t *d_sec;			/* Device secret */
} auth;

static struct {
	bool send_in_progress;		/* Set if a message is currently being sent */
	const cc_buffer_desc *buf;	/* Outgoing data buffer */
	cc_callback_rtn cb;		/* "Send" callback */
} conn_out;

static struct {
	bool recv_in_progress;		/* Set if a receive was scheduled */
	cc_buffer_desc *buf;		/* Incoming data buffer */
	cc_callback_rtn cb;		/* "Receive" callback */
} conn_in;

static struct {
	bool conn_done;			/* TCP connection was established */
	bool auth_done;			/* Auth was sent for this session */
	bool pend_bit;			/* Cloud has a pending message */
	bool pend_ack;			/* Set if the device needs to ACK prev msg */
	bool nack_sent;			/* Set if a NACK was sent from the device */
	char host[MAX_HOST_LEN + 1];	/* Store the host name */
	char port[MAX_PORT_LEN + 1];	/* Store the host port */
} session;

static struct {
	uint32_t start_ts;		/* Polling interval measurement starts
					 * from this timestamp */
	int32_t polling_int_ms;		/* Polling interval in milliseconds */
	bool new_interval_set;		/* Set if a new interval was received */
} timekeep;

#define INVOKE_SEND_CALLBACK(_buf, _evt) \
	do { \
		if (conn_out.cb) \
			conn_out.cb((cc_buffer_desc *)(_buf), (_evt)); \
	} while(0)

#define INVOKE_RECV_CALLBACK(_buf, _evt) \
	do { \
		if (conn_in.cb) \
			conn_in.cb((cc_buffer_desc *)(_buf), (_evt)); \
	} while(0)

/* Reset the connection (incoming and outgoing) and session structures */
static inline void reset_conn_and_session_states(void)
{
	conn_out.send_in_progress = false;
	conn_out.buf = NULL;
	conn_out.cb = NULL;
	session.conn_done = false;
	session.auth_done = false;
	session.pend_bit = false;
	session.pend_ack = false;
	session.nack_sent = false;
}

/* Called on receiving a quit; Close the connection and reset the state */
static inline void on_recv_quit(void)
{
	ott_close_connection();
	reset_conn_and_session_states();
}

/* Send a QUIT or NACK + QUIT and then close the connection and reset the state */
static inline void initiate_quit(bool send_nack)
{
	if (!session.conn_done)
		return;
	c_flags_t c_flags = send_nack ? (CF_NACK | CF_QUIT) : CF_QUIT;
	ott_send_ctrl_msg(c_flags);
	ott_close_connection();
	reset_conn_and_session_states();
}

static inline void init_state(void)
{
	reset_conn_and_session_states();
	session.host[0] = 0x00;
	session.port[0] = 0x00;
	timekeep.start_ts = 0;
	timekeep.polling_int_ms = INIT_POLLLING_MS;
	timekeep.new_interval_set = true;
}

bool cc_init(const uint8_t *d_ID, uint16_t d_sec_sz, const uint8_t *d_sec)
{
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

uint32_t cc_get_sleep_interval(const cc_buffer_desc *buf)
{
	m_type_t m_type;
	const msg_t *ptr_to_msg = (const msg_t *)(buf->buf_ptr);
	OTT_LOAD_MTYPE(ptr_to_msg->cmd_byte, m_type);

	if (m_type == MT_CMD_SL)
		return ptr_to_msg->data.interval;
	else
		return 0;
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

	strncpy(session.host, host, sizeof(session.host));
	strncpy(session.port, port, sizeof(session.port));

	return true;
}

void cc_ack_bytes(void)
{
	session.pend_ack = true;
}

void cc_nak_bytes(void)
{
	initiate_quit(true);
	session.nack_sent = true;
}

/*
 * Process a received response. Most of the actual processing is done through the
 * user provided callbacks this function invokes. In addition, it sets some
 * internal flags to decide the state of the session in the future. Calling the
 * send callback is optional and is decided through "invoke_send_cb".
 * On receiving a NACK return "false". Otherwise, return "true".
 */
static bool process_recvd_msg(msg_t *msg_ptr, bool invoke_send_cb)
{
	c_flags_t c_flags;
	m_type_t m_type;
	bool no_nack_detected = true;

	OTT_LOAD_FLAGS(msg_ptr->cmd_byte, c_flags);
	OTT_LOAD_MTYPE(msg_ptr->cmd_byte, m_type);

	/* Keep the session alive if the cloud has more messages to send */
	session.pend_bit = OTT_FLAG_IS_SET(c_flags, CF_PENDING);

	if (OTT_FLAG_IS_SET(c_flags, CF_ACK)) {
		cc_event evt = CC_STS_NONE;
		/* Messages with a body need to be ACKed in the future */
		session.pend_ack = false;
		if (m_type == MT_UPDATE) {
			evt = CC_STS_RCV_UPD;
		} else if (m_type == MT_CMD_SL) {
			evt = CC_STS_RCV_CMD_SL;
		} else if (m_type == MT_CMD_PI) {
			timekeep.polling_int_ms = msg_ptr->data.interval * MULT;
			timekeep.new_interval_set = true;
			session.pend_ack = true;
		}

		/* Call the callbacks with the type of message received */
		if (m_type == MT_UPDATE || m_type == MT_CMD_SL) {
			conn_in.recv_in_progress = false;
			INVOKE_RECV_CALLBACK(conn_in.buf, evt);
		}
		if (invoke_send_cb)
			INVOKE_SEND_CALLBACK(conn_out.buf, CC_STS_ACK);
	} else if (OTT_FLAG_IS_SET(c_flags, CF_NACK)) {
		no_nack_detected = false;
		if (invoke_send_cb)
			INVOKE_SEND_CALLBACK(conn_out.buf, CC_STS_NACK);
	}

	if (OTT_FLAG_IS_SET(c_flags, CF_QUIT))
		on_recv_quit();

	return no_nack_detected;
}

/*
 * Attempt to receive a response within a timeout. Once a response is received,
 * process it. This helper function must be called everytime the device sends
 * a message to the cloud and expects a response in return.
 * "invoke_send_cb" decides if the send callback should be invoked on a valid
 * response. When expecting a response for an auth message (or any internal
 * message), this should be false.
 * On successfully receiving a valid response, this function will return "true".
 * On receiving a NACK or timeout, return "false".
 */
static bool recv_resp_within_timeout(uint32_t timeout, bool invoke_send_cb)
{
	conn_out.send_in_progress = true;

	uint32_t start = platform_get_tick_ms();
	uint32_t end = start;
	bool no_nack;
	msg_t *msg_ptr = (msg_t *)conn_in.buf->buf_ptr;
	do {
		ott_status s = ott_retrieve_msg(msg_ptr, conn_in.buf->bufsz);

		if (s == OTT_INV_PARAM || s == OTT_ERROR) {
			conn_out.send_in_progress = false;
			return false;
		}
		if (s == OTT_NO_MSG) {
			end = platform_get_tick_ms();
			continue;
		}
		if (s == OTT_OK) {
			no_nack = process_recvd_msg(msg_ptr, invoke_send_cb);
			break;
		}
	} while(end - start < timeout);

	conn_out.send_in_progress = false;

	if (end - start >= timeout) {
		if (invoke_send_cb)
			INVOKE_SEND_CALLBACK(conn_out.buf, CC_STS_SEND_TIMEOUT);
		return false;
	}

	return no_nack;
}

static bool establish_session(const char *host, const char *port, bool polling)
{
	if (host == NULL || port == NULL || conn_in.buf == NULL ||
			conn_in.buf->buf_ptr == NULL ||
			!conn_in.recv_in_progress)
		return false;

retry_connection:
	if (ott_initiate_connection(host, port) != OTT_OK)
		return false;

	session.conn_done = true;
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

	/* This call should not invoke the send callback */
	if (!recv_resp_within_timeout(RECV_TIMEOUT_MS, false)) {
		initiate_quit(true);
		return false;
	}

	/* If we NACKed an incoming message, the session was ended. Retry. */
	if (session.nack_sent) {
		session.nack_sent = false;
		goto retry_connection;
	}

	/* If neither side has nothing to send while polling, the connection
	 * would have been terminated in recv_resp_within_timeout().
	 */
	if (polling && !session.conn_done) {
		session.auth_done = false;
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
}

cc_send_result cc_send_bytes_to_cloud(const cc_buffer_desc *buf, cc_data_sz sz,
		cc_callback_rtn cb)
{
	if (conn_out.send_in_progress)
		return CC_SEND_BUSY;

	if (!buf || !buf->buf_ptr || sz == 0)
		return CC_SEND_FAILED;

	if (strlen(session.host) == 0 || strlen(session.port) == 0)
		return CC_SEND_FAILED;

	if (!conn_in.recv_in_progress)
		return CC_SEND_FAILED;

	/*
	 * If a session hasn't been established, initiate a connection. This
	 * leads to the device being authenticated.
	 */
	if (!session.auth_done)
		if (!establish_session(session.host, session.port, false))
			return CC_SEND_FAILED;

	/*
	 * If the user ACKed a previous message, set the flag in this message.
	 * Also, make sure the PENDING flag is always set so that the device has
	 * full control on when to disconnect.
	 */
	c_flags_t c_flags = session.pend_ack ? (CF_PENDING | CF_ACK) :
		CF_PENDING;
	if (ott_send_status_to_cloud(c_flags, sz,
				(const uint8_t *)buf->buf_ptr) != OTT_OK) {
		initiate_quit(false);
		return CC_SEND_FAILED;
	}

	conn_out.cb = cb;
	conn_out.buf = buf;
	session.pend_ack = false;

	/* Receive a message within a timeout and invoke the send callback */
	if (!recv_resp_within_timeout(RECV_TIMEOUT_MS, true)) {
		initiate_quit(true);
		return CC_SEND_FAILED;
	}

	if (session.nack_sent)
		session.nack_sent = false;

	return CC_SEND_SUCCESS;
}

cc_send_result cc_resend_init_config(cc_callback_rtn cb)
{
	if (conn_out.send_in_progress)
		return CC_SEND_BUSY;

	if (strlen(session.host) == 0 || strlen(session.port) == 0)
		return CC_SEND_FAILED;

	if (!conn_in.recv_in_progress)
		return CC_SEND_FAILED;

	/*
	 * If a session hasn't been established, initiate a connection. This
	 * leads to the device being authenticated.
	 */
	if (!session.auth_done)
		if (!establish_session(session.host, session.port, false))
			return CC_SEND_FAILED;

	/*
	 * If the user ACKed a previous message, set the flag in this message.
	 * Also, make sure the PENDING flag is always set so that the device has
	 * full control on when to disconnect.
	 */
	c_flags_t c_flags = session.pend_ack ? (CF_PENDING | CF_ACK) :
		CF_PENDING;
	if (ott_send_restarted(c_flags) != OTT_OK) {
		initiate_quit(false);
		return CC_SEND_FAILED;
	}

	conn_out.cb = cb;
	conn_out.buf = NULL;
	session.pend_ack = false;

	/* Receive a message within a timeout and invoke the send callback */
	if (!recv_resp_within_timeout(RECV_TIMEOUT_MS, true)) {
		initiate_quit(true);
		return CC_SEND_FAILED;
	}

	if (session.nack_sent)
		session.nack_sent = false;

	return CC_SEND_SUCCESS;
}

cc_recv_result cc_recv_bytes_from_cloud(cc_buffer_desc *buf, cc_callback_rtn cb)
{
	if (!buf || !buf->buf_ptr)
		return CC_RECV_FAILED;

	if (conn_in.recv_in_progress)
		return CC_RECV_BUSY;

	conn_in.recv_in_progress = true;
	conn_in.buf = buf;
	conn_in.cb = cb;

	memset(buf->buf_ptr, 0, buf->bufsz + OTT_OVERHEAD_SZ);
	return CC_RECV_SUCCESS;
}

/*
 * Receive any pending messages from the cloud or ACK any previous message. If
 * the message was to be NACKed, it would have been already done through the
 * receive callback.
 */
static void poll_cloud_for_messages(void)
{
	while (session.pend_bit || session.pend_ack) {
		c_flags_t c_flags = session.pend_ack ? CF_ACK : CF_NONE;
		c_flags |= ((!session.pend_bit) ? CF_QUIT : CF_NONE);
		ott_send_ctrl_msg(c_flags);
		if (!session.pend_bit) {	/* If last message in session */
			ott_close_connection();
			reset_conn_and_session_states();
			return;
		}

		if (!recv_resp_within_timeout(RECV_TIMEOUT_MS, true)) {
			initiate_quit(true);
			return;
		}

		if (session.nack_sent)
			session.nack_sent = false;
	}
}

/*
 * This call retrieves any pending messages the cloud has to send. It also
 * polls the cloud if the polling interval was hit and updates the time when the
 * cloud should be called next.
 */
int32_t cc_service_send_receive(uint32_t cur_ts)
{
	int32_t next_call_time_ms = -1;
	bool polling_due = cur_ts - timekeep.start_ts >= timekeep.polling_int_ms;

	/*
	 * Poll for messages / ACK any previous messages if the connection is
	 * still open or if the polling interval was hit.
	 */
	if (session.auth_done || polling_due) {
		if (!session.auth_done)
			if (!establish_session(session.host, session.port, true))
				goto next_wakeup;
		poll_cloud_for_messages();
	}

next_wakeup:
	/* Compute when this function needs to be called next */
	if (polling_due || timekeep.new_interval_set) {
		timekeep.new_interval_set = false;
		next_call_time_ms = timekeep.polling_int_ms;
		timekeep.start_ts = cur_ts;
	}

	close_session();
	return next_call_time_ms;
}

void cc_interpret_msg(const cc_buffer_desc *buf, uint8_t tab_level)
{
	if (!buf || !buf->buf_ptr)
		return;
	ott_interpret_msg((msg_t *)buf->buf_ptr, tab_level);
}
