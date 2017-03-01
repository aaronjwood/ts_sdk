/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __CL_PROTOCOL_INT
#define __CL_PROTOCOL_INT

#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"

#if defined (OTT_PROTOCOL)
#include "ott_protocol.h"
#elif defined (SMSNAS_PROTOCOL)
#include "smsnas_protocol.h"
#else
#error "define valid protocol options from OTT_PROTOCOL or SMSNAS_PROTOCOL"
#endif

/* Very thin layer sits between cloud_comm api and undelying protocol depending
 * on the compilation switch
 */

#if defined (OTT_PROTOCOL)

#define PROTO_INIT() do { \
        if (ott_protocol_init() != PROTO_OK) \
                return false; \
} while(0)

#define PROTO_GET_RCVD_MSG_PTR(msg) ott_get_rcv_buffer_ptr((msg))

#define PROTO_SET_DESTINATION(dest) do { \
        if (ott_set_destination((dest)) != PROTO_OK) \
		return false; \
} while(0)

#define PROTO_SET_AUTH(d_id, d_id_sz, d_sec, d_sec_sz) do { \
        if (ott_set_auth((d_id), (d_id_sz), (d_sec), (d_sec_sz)) != PROTO_OK) \
		return false; \
} while(0)

#define PROTO_GET_POLLING() ott_get_polling_interval()
#define PROTO_SET_POLLING(new_value) ott_set_polling_interval((new_value));

#define PROTO_INITIATE_QUIT(send_nack) do { \
        ott_initiate_quit((send_nack)); \
} while(0)

#define PROTO_SEND_MSG_TO_CLOUD(msg, sz, svc_id, cb) do {		\
		if (ott_send_msg_to_cloud((msg), (sz), (svc_id), \
					  (cc_send_cb)) != PROTO_OK) {	\
		reset_conn_states(); \
		return CC_SEND_FAILED; \
	} \
} while(0)

#define PROTO_SEND_ACK() ott_send_ack()
#define PROTO_SEND_NACK() ott_send_nack()

#define PROTO_SET_RECV_BUFFER_CB(rcv_buf, sz, rcv_cb) do { \
        if (ott_set_recv_buffer_cb((rcv_buf), (sz), (rcv_cb)) != PROTO_OK) \
                return CC_RECV_FAILED; \
} while(0)

#define PROTO_MAINTENANCE(poll_due, cur_ts) do { \
        (void)(cur_ts); \
        ott_maintenance((poll_due)); \
} while(0)

/* SMSNAS protocol defines */
#elif defined (SMSNAS_PROTOCOL)

#define PROTO_INIT() do { \
        if (smsnas_protocol_init() != PROTO_OK) \
                return false; \
} while(0)

#define PROTO_GET_RCVD_MSG_PTR(msg) smsnas_get_rcv_buffer_ptr((msg))

#define PROTO_SET_DESTINATION(dest) do { \
        if (smsnas_set_destination((dest)) != PROTO_OK) \
		return false; \
} while(0)

#define PROTO_GET_POLLING() smsnas_get_polling_interval()

#define PROTO_INITIATE_QUIT(send_nack)

#define PROTO_SEND_MSG_TO_CLOUD(msg, sz, svc_id, cb) do {		\
		if (smsnas_send_msg_to_cloud((msg), (sz), (svc_id), \
					  (cc_send_cb)) != PROTO_OK) {	\
		reset_conn_states(); \
		return CC_SEND_FAILED; \
	} \
} while(0)

#define PROTO_SEND_ACK() smsnas_send_ack()
#define PROTO_SEND_NACK() smsnas_send_nack()

#define PROTO_SET_RECV_BUFFER_CB(rcv_buf, sz, rcv_cb) do { \
        if (smsnas_set_recv_buffer_cb((rcv_buf), (sz), (rcv_cb)) != PROTO_OK) \
                return CC_RECV_FAILED; \
} while(0)

#define PROTO_MAINTENANCE(poll_due, cur_ts) do { \
        smsnas_maintenance((poll_due), (cur_ts)); \
} while(0)

#else
#error "define valid protocol options from OTT_PROTOCOL or SMSNAS_PROTOCOL"

#endif /* OTT_PROTOCOL ifelse ends */

#endif
