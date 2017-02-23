/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>
#include "platform.h"
#include "cloud_comm_def.h"
#include "cloud_protocol_intfc.h"
#include "service_common.h"
#include "cc_control_service.h"
#include "dbg.h"

/* Default cloud polling time in miliseconds if supported by the protocol */
uint32_t init_polling_ms;

#ifdef PROTO_TIME_PROFILE
static uint32_t proto_begin;
#endif

typedef struct service_dispatch_entry {
	const cc_service_descriptor *descriptor;
	cc_svc_callback_rtn app_callback;
} service_dispatch_entry;

/* XXX Only two services for now so we use a fixed array for dispatching. */
static service_dispatch_entry service_table[2];

static void dispatch_event_to_service(cc_service_id svc_id, cc_buffer_desc *buf,
				      cc_event event);
static cc_set_recv_result activate_buffer_for_recv(cc_buffer_desc *buf);

/* Reset the connection (incoming and outgoing) and session structures */
static inline void reset_conn_states(void)
{
	conn_out.send_in_progress = false;
	conn_out.buf = NULL;
}

/* Receive callback invoked by the protocol layer */
static void cc_recv_cb(const void *buf, uint32_t sz,
		       proto_event event, cc_service_id svc_id)
{
	switch(event) {
	case PROTO_RCVD_MSG:
		conn_in.buf->current_len = sz;
		dispatch_event_to_service(svc_id, conn_in.buf, CC_EVT_RCVD_MSG);
		conn_in.recv_in_progress = false;
		activate_buffer_for_recv(conn_in.buf);
		break;
	case PROTO_RCVD_QUIT:
		reset_conn_states();
		break;
	default:
		dbg_printf("%s:%d: Unknown Rcvd Event\n", __func__, __LINE__);
		break;
	}

}

/* Send callback invoked by the protocol layer */
static void cc_send_cb(const void *buf, uint32_t sz, proto_event event,
		       cc_service_id svc_id)
{
	cc_event ev = CC_EVT_NONE;
	switch(event) {
	case PROTO_RCVD_ACK:
		ev = CC_EVT_SEND_ACKED;
		break;
	case PROTO_RCVD_NACK:
		ev = CC_EVT_SEND_NACKED;
		break;
	case PROTO_SEND_TIMEOUT:
		ev = CC_EVT_SEND_TIMEOUT;
		break;
	default:
		dbg_printf("%s:%d: Unknown Send Event\n", __func__, __LINE__);
		break;
	}
	dispatch_event_to_service(svc_id, conn_out.buf, ev);
	activate_buffer_for_recv(conn_in.buf);
}

static inline void init_state(void)
{
	reset_conn_states();
	timekeep.start_ts = 0;
	timekeep.polling_int_ms = init_polling_ms;
}

bool cc_init(cc_svc_callback_rtn control_cb)
{
	PROTO_INIT();
	init_state();
	init_polling_ms = PROTO_GET_DEFAULT_POLLING();
	memset(service_table, 0, sizeof(service_table));
	
	/* The Control service must always be registered. */
	return cc_register_service(&cc_control_service_descriptor, control_cb);
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
	 * Depending on the type of message and protocol, return
	 * pointer to binary data
	 */
	PROTO_GET_RCVD_MSG_PTR(buf->buf_ptr);

}

cc_data_sz cc_get_receive_data_len(const cc_buffer_desc *buf)
{
	/* Check for non-NULL buffer pointers */
	if (!buf || !buf->buf_ptr)
		return 0;
	return buf->current_len - PROTO_OVERHEAD_SZ;
}

bool cc_adjust_buffer_desc(cc_buffer_desc *buf, int adj)
{
	uint8_t *tmp = buf->buf_ptr;

	if (buf->current_len < adj)
		return false;

	buf->buf_ptr = (void *)(tmp + adj);
	buf->current_len -= adj;
	return true;
}

bool cc_set_destination(const char *dest)
{
	PROTO_SET_DESTINATION(dest);
	return true;
}

bool cc_set_auth_credentials(const uint8_t *d_id, uint32_t d_id_sz,
				const uint8_t *d_sec, uint32_t d_sec_sz)
{

	PROTO_SET_AUTH(d_id, d_id_sz, d_sec, d_sec_sz);
	return true;
}

void cc_ack_msg(void)
{
	PROTO_SEND_ACK();
}

void cc_nak_msg(void)
{
	PROTO_SEND_NACK();
}

cc_send_result cc_send_svc_msg_to_cloud(cc_buffer_desc *buf,
					cc_data_sz sz, cc_service_id svc_id)
{
	conn_out.buf = NULL;

	if (conn_out.send_in_progress)
		return CC_SEND_BUSY;

	if (!buf || !buf->buf_ptr || sz == 0)
		return CC_SEND_FAILED;

	if (!conn_in.recv_in_progress)
		return CC_SEND_FAILED;

	conn_out.send_in_progress = true;
	conn_out.buf = buf;
	PROTO_SEND_MSG_TO_CLOUD(buf->buf_ptr, sz, svc_id, cc_send_cb);

	conn_out.send_in_progress = false;
	return CC_SEND_SUCCESS;
}

static cc_set_recv_result activate_buffer_for_recv(cc_buffer_desc *buf)
{
	memset(buf->buf_ptr, 0, buf->bufsz + PROTO_OVERHEAD_SZ);
	PROTO_SET_RECV_BUFFER_CB(buf->buf_ptr, buf->bufsz + PROTO_OVERHEAD_SZ,
				cc_recv_cb);

	conn_in.recv_in_progress = true;
	conn_in.buf = buf;
	return CC_RECV_SUCCESS;
}

cc_set_recv_result cc_set_recv_buffer(cc_buffer_desc *buf)
{
	if (!buf || !buf->buf_ptr)
		return CC_RECV_FAILED;

	if (conn_in.recv_in_progress)
		return CC_RECV_BUSY;

	return activate_buffer_for_recv(buf);
}

uint32_t cc_service_send_receive(uint32_t cur_ts)
{
	uint32_t next_call_time_ms;
	bool polling_due = cur_ts - timekeep.start_ts >= timekeep.polling_int_ms;

	PROTO_MAINTENANCE(polling_due);

	/* Compute when this function needs to be called next */
	timekeep.polling_int_ms = PROTO_GET_POLLING();
	if (polling_due) {
		next_call_time_ms = timekeep.polling_int_ms;
		timekeep.start_ts = cur_ts;
	} else
		next_call_time_ms = timekeep.start_ts +
			timekeep.polling_int_ms - cur_ts;

	PROTO_INITIATE_QUIT(false);
	reset_conn_states();
	return next_call_time_ms;
}

bool cc_register_service(const cc_service_descriptor *svc_desc,
			 cc_svc_callback_rtn cb)
{
	cc_service_id svc_id;

	if (svc_desc == NULL  || svc_desc->dispatch_callback == NULL ||
	    cb == NULL)
		return false;

	svc_id = svc_desc->svc_id;

	if (svc_id != CC_SERVICE_CONTROL && svc_id != CC_SERVICE_BASIC)
		return false;

	service_table[svc_id].descriptor = svc_desc;
	service_table[svc_id].app_callback = cb;
	return true;
}

static void dispatch_event_to_service(cc_service_id svc_id, cc_buffer_desc *buf,
				      cc_event event)
{
	const cc_service_descriptor *desc;

	/* Ignore events for unsupported service ids. */
	if (svc_id != CC_SERVICE_CONTROL && svc_id != CC_SERVICE_BASIC) {
		if (event == CC_EVT_RCVD_MSG) {
			cc_ack_msg();
			return;
		}
	}
	desc = service_table[svc_id].descriptor;
	desc->dispatch_callback(buf, event, svc_id,
				service_table[svc_id].app_callback);
}
