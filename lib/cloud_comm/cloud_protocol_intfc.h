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
#endif

/* Very thin layer sits between cloud_comm api and undelying protocol depending
 * on the compilation switch
 */

#if defined (OTT_PROTOCOL)

#define PROTO_INIT() do { \
        if (ott_protocol_init() != PROTO_OK) \
                return false; \
} while(0)

#define PROTO_SET_DESTINATION(host, port) do { \
        if (ott_set_destination((host), (port)) != PROTO_OK) \
		return false; \
} while(0)

#define PROTO_SET_AUTH(d_id, d_id_sz, d_sec, d_sec_sz) do { \
        if (ott_set_auth((d_id), (d_id_sz), (d_sec), (d_sec_sz)) != PROTO_OK) \
		return false; \
} while(0)

#define PROTO_GET_DEFAULT_POLLING() ott_get_polling_interval(NULL, true)
#define PROTO_GET_POLLING(msg) ott_get_polling_interval((msg), false)

#define PROTO_INITIATE_QUIT(send_nack) do { \
        ott_initiate_quit((send_nack)); \
} while(0)

#define PROTO_GET_SLEEP_INTERVAL(msg) return ott_get_sleep_interval((msg))
#define PROTO_RESEND_INIT_CONFIG(cb) do { \
        if (ott_resend_init_config((cb)) != PROTO_OK) { \
		reset_conn_states(); \
		return CC_SEND_FAILED; \
	} \
} while(0)

#define PROTO_SEND_MSG_TO_CLOUD(msg, sz, cb) do { \
        if (ott_send_msg_to_cloud((msg), (sz), (cc_send_cb)) != PROTO_OK) { \
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

#define PROTO_MAINTENANCE(poll_due) ott_maintenance((poll_due))

#elif defined (SMSNAS_PROTOCOL)

#define PROTO_INIT()
#define PROTO_SET_DESTINATION(host, port)
#define PROTO_SET_AUTH(d_id, d_id_sz, d_sec, d_sec_sz)
#define PROTO_GET_DEFAULT_POLLING()
#define PROTO_GET_POLLING(msg)
#define PROTO_INITIATE_QUIT(send_nack)
#define PROTO_GET_SLEEP_INTERVAL(msg)
#define PROTO_RESEND_INIT_CONFIG(cb)
#define PROTO_SEND_MSG_TO_CLOUD(msg, sz, cb)
#define PROTO_SEND_ACK()
#define PROTO_SEND_NACK()
#define PROTO_SET_RECV_BUFFER_CB(rcv_buf, sz, rcv_cb)
#define PROTO_MAINTENANCE(poll_due)

#else
Please define protocol to use, valid options are OTT_PROTOCOL and SMSNAS_PROTOCOL

#endif /* OTT_PROTOCOL ifelse ends */

#endif
