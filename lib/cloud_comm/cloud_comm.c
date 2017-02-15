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

#define INVOKE_SEND_CALLBACK(_buf, _evt, _svc_id)	\
	do { \
		if (conn_out.cb) \
			conn_out.cb((cc_buffer_desc *)(_buf), (_evt), \
				    (_svc_id));			      \
	} while(0)

#define INVOKE_RECV_CALLBACK(_buf, _evt, _svc_id)	\
	do { \
		if (conn_in.cb) \
			conn_in.cb((cc_buffer_desc *)(_buf), (_evt), \
				(_svc_id));                           \
	} while(0)

/* Reset the connection (incoming and outgoing) and session structures */
static inline void reset_conn_states(void)
{
	conn_out.send_in_progress = false;
	conn_out.buf = NULL;
	conn_out.cb = NULL;
}

/* Receive callback invoked by the protocol layer */
static void cc_recv_cb(const void *buf, uint32_t sz,
		       proto_event event, cc_service_id svc_id)
{
	switch(event) {
	case PROTO_RCVD_MSG:
		conn_in.buf->current_len = sz;
		INVOKE_RECV_CALLBACK(conn_in.buf, CC_EVT_RCVD_MSG, svc_id);
		conn_in.recv_in_progress = false;
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
	INVOKE_SEND_CALLBACK(conn_out.buf, ev, svc_id);
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

uint32_t cc_get_sleep_interval(const cc_buffer_desc *buf)
{
	if (!buf || !buf->buf_ptr)
		return 0;
	PROTO_GET_SLEEP_INTERVAL(buf->buf_ptr);
}

cc_data_sz cc_get_receive_data_len(const cc_buffer_desc *buf)
{
	/* Check for non-NULL buffer pointers */
	if (!buf || !buf->buf_ptr)
		return 0;
	return buf->current_len - PROTO_OVERHEAD_SZ;
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

cc_send_result cc_send_svc_msg_to_cloud(const cc_buffer_desc *buf,
					cc_data_sz sz, cc_service_id svc_id,
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
	conn_out.cb = cb;
	conn_out.buf = buf;
	PROTO_SEND_MSG_TO_CLOUD(buf->buf_ptr, sz, svc_id, cc_send_cb);

	conn_out.send_in_progress = false;
	return CC_SEND_SUCCESS;
}

cc_recv_result cc_recv_msg_from_cloud(cc_buffer_desc *buf, cc_callback_rtn cb)
{
	if (!buf || !buf->buf_ptr)
		return CC_RECV_FAILED;

	if (conn_in.recv_in_progress)
		return CC_RECV_BUSY;

	memset(buf->buf_ptr, 0, buf->bufsz + PROTO_OVERHEAD_SZ);
	PROTO_SET_RECV_BUFFER_CB(buf->buf_ptr, buf->bufsz + PROTO_OVERHEAD_SZ,
				cc_recv_cb);

	conn_in.recv_in_progress = true;
	conn_in.buf = buf;
	conn_in.cb = cb;
	return CC_RECV_SUCCESS;
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

static cc_svc_callback_rtn __apps_control_service_callback;
static cc_service_dispatch_callback __control_service_dispatch;

bool cc_register_service(cc_service_descriptor *svc_desc,
			 cc_svc_callback_rtn cb)
{
	if (svc_desc == NULL  || svc_desc->dispatch_callback == NULL ||
	    cb == NULL)
		return false;
	/*
	 * XXX This should build a lookup table of all the registered services,
	 * but for now we only have the Control service. As a temporary hack
	 * stash the info needed to dispatch service messages into static
	 * variables.
	 */
	if (svc_desc->svc_id != CC_SERVICE_CONTROL)
		return false;
	__control_service_dispatch = svc_desc->dispatch_callback;
	__apps_control_service_callback = cb;

	return true;
}

void cc_dispatch_event_to_service(cc_service_id svc_id, cc_buffer_desc *buf,
				  cc_event event)
{
	/* Lookup the service id.  Find it's receive entry point.
	   Call that entry point with the event and message for the service.
	   Pass along the callback registered by the application in case
	   the service needs to deliver it an event. */

	/* XXX Only do Control service for now. */
	if (svc_id != CC_SERVICE_CONTROL)
		return;
	__control_service_dispatch(buf, event,CC_SERVICE_CONTROL,
				   __apps_control_service_callback);
}


void cc_unregister_service(cc_service_descriptor *svc_desc)
{
	/* XXX Stub for now */
}
