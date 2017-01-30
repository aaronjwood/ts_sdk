/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>
#include "protocol.h"
#include "platform.h"
#include "cloud_comm.h"
#include "cloud_comm_private.h"
#include "cloud_comm_def.h"

#include "dbg.h"

uint32_t init_polling_ms;

#ifdef PROTO_TIME_PROFILE
static uint32_t proto_begin;
#endif

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

/* Receive callback */
static void cc_recv_cb(const void *buf, proto_event event)
{
	switch(event) {
	case PROTO_RCVD_UPD:
		INVOKE_RECV_CALLBACK(buf, CC_STS_RCV_UPD);
		conn_in.recv_in_progress = false;
		break;
	case PROTO_RCVD_CMD_SL:
		INVOKE_RECV_CALLBACK(buf, CC_STS_RCV_CMD_SL);
		conn_in.recv_in_progress = false;
		break;
	case PROTO_RCVD_CMD_PI:
		timekeep.polling_int_ms = *((uint32_t *)buf);
		timekeep.new_interval_set = true;
		break;
	case PROTO_RCVD_QUIT:
		reset_conn_states();
		break;
	default:
		dbg_printf("%s:%d: Unknown Rcvd Event\n", __func__, __LINE__);
		INVOKE_RECV_CALLBACK(NULL, CC_STS_UNKNOWN);
		break;
	}

}

/* Send callback */
static void cc_send_cb(const void *buf, proto_event event)
{
	cc_event ev;
	switch(event) {
	case PROTO_RCVD_ACK:
		ev = CC_STS_ACK;
		break;
	case PROTO_RCVD_NACK:
		ev = CC_STS_NACK;
		break;
	case PROTO_SEND_TIMEOUT;
		ev = CC_STS_SEND_TIMEOUT;
		break;
	default:
		dbg_printf("%s:%d: Unknown Send Event\n", __func__, __LINE__);
		ev = CC_STS_UNKNOWN;
		break;
	}
	INVOKE_SEND_CALLBACK(buf, ev);

}

/* Reset the connection (incoming and outgoing) and session structures */
static inline void reset_conn_states(void)
{
	conn_out.send_in_progress = false;
	conn_out.buf = NULL;
	conn_out.cb = NULL;
}

static inline void init_state(void)
{
	reset_conn_states();
	timekeep.start_ts = 0;
	timekeep.polling_int_ms = init_polling_ms;
	timekeep.new_interval_set = true;
}

bool cc_init()
{
	if (proto_init() != PROTO_OK)
		return false;
	init_state();
	init_polling_ms = proto_get_default_polling();
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
	 * Depending on the type of message in the buffer and protocol, return
	 * pointer to binary data
	 */
	return proto_get_rcvd_msg_ptr(buf->buf_ptr);

}

uint32_t cc_get_sleep_interval(const cc_buffer_desc *buf)
{
	if (!buf || !buf->buf_ptr)
		return NULL;
	return proto_get_sleep_interval(buf->buf_ptr);
}

cc_data_sz cc_get_receive_data_len(const cc_buffer_desc *buf)
{
	/* Check for non-NULL buffer pointers */
	if (!buf || !buf->buf_ptr)
		return 0;
	return proto_get_rcvd_data_len(buf->buf_ptr);
}

bool cc_set_destination(const char *host, const char *port)
{
	if (proto_set_destination(host, port) != PROTO_OK)
		return false;
	return true;
}

bool cc_set_auth_credentials(const uint8_t *d_ID, uint16_t d_sec_sz,
			const uint8_t *d_sec)
{

	if (proto_set_auth(d_ID, d_sec_sz, d_sec) != PROTO_OK)
		return false;
	return true;
}

void cc_ack_bytes(void)
{
	proto_send_ack();
}

void cc_nak_bytes(void)
{
	proto_send_nack();
}

cc_send_result cc_send_msg_to_cloud(const cc_buffer_desc *buf, cc_data_sz sz,
		cc_callback_rtn cb)
{
	conn_out.cb = NULL;
	conn_out.buf = NULL;

	if (conn_out.send_in_progress)
		return CC_SEND_BUSY;

	if (!buf || !buf->buf_ptr || sz == 0)
		return CC_SEND_FAILED;

	if (!conn_in.recv_in_progress)
		return CC_SEND_FAILED;

	conn_out.send_in_progress = true;
	if (proto_send_msg_to_cloud(buf->buf_ptr, sz, cc_send_cb) != PROTO_OK) {
		reset_conn_states();
		return CC_SEND_FAILED;
	}

	conn_out.cb = cb;
	conn_out.buf = buf;

	return CC_SEND_SUCCESS;
}

cc_send_result cc_resend_init_config(cc_callback_rtn cb)
{
	if (conn_out.send_in_progress)
		return CC_SEND_BUSY;

	if (!conn_in.recv_in_progress)
		return CC_SEND_FAILED;

	conn_out.send_in_progress = true;

	if (proto_resend_init_config(cb) != PROTO_OK) {
		reset_conn_states();
		return CC_SEND_FAILED;
	}

	conn_out.cb = cb;
	conn_out.buf = NULL;

	return CC_SEND_SUCCESS;
}

cc_recv_result cc_recv_bytes_from_cloud(cc_buffer_desc *buf, cc_callback_rtn cb)
{
	if (!buf || !buf->buf_ptr)
		return CC_RECV_FAILED;

	if (conn_in.recv_in_progress)
		return CC_RECV_BUSY;

	memset(buf->buf_ptr, 0, buf->bufsz + PROTO_OVERHEAD_SZ);
	proto_result res = proto_set_recv_buffer_cb(buf->buf_ptr,
						buf->bufsz + PROTO_OVERHEAD_SZ,
						cc_recv_cb);
	if (res != PROTO_OK)
		return CC_RECV_FAILED;

	conn_in.recv_in_progress = true;
	conn_in.buf = buf;
	conn_in.cb = cb;
	return CC_RECV_SUCCESS;
}


/*
 * This call retrieves any pending messages the cloud has to send. It also
 * polls the cloud if the polling interval was hit and updates the time when the
 * cloud should be called next.
 */
uint32_t cc_service_send_receive(uint32_t cur_ts)
{
	uint32_t next_call_time_ms =
		timekeep.start_ts + timekeep.polling_int_ms - cur_ts;
	bool polling_due = cur_ts - timekeep.start_ts >= timekeep.polling_int_ms;

	/*
	 * Poll for messages / ACK any previous messages if the connection is
	 * still open or if the polling interval was hit.
	 */
	proto_maintenance(polling_due);

	/* Compute when this function needs to be called next */
	if (polling_due || timekeep.new_interval_set) {
		timekeep.new_interval_set = false;
		next_call_time_ms = timekeep.polling_int_ms;
		timekeep.start_ts = cur_ts;
	}

	proto_initiate_quit(false);
	reset_conn_states();
	return next_call_time_ms;
}

void cc_interpret_msg(const cc_buffer_desc *buf, uint8_t tab_level)
{
	if (!buf || !buf->buf_ptr)
		return;
	ott_interpret_msg((msg_t *)buf->buf_ptr, tab_level);
}
