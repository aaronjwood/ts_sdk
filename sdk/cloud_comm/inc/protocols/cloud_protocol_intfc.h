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
#elif defined (MQTT_PROTOCOL)
#include "mqtt_protocol.h"
#else
#error "define valid protocol options from OTT_PROTOCOL/SMSNAS_PROTOCOL/MQTT_PROTOCOL"
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

#define PROTO_SET_OWN_AUTH(d_id, d_id_sz, d_sec, d_sec_sz) do { \
        if (ott_set_own_auth((d_id), (d_id_sz), (d_sec), (d_sec_sz)) != PROTO_OK) \
		return false; \
} while(0)

#define PROTO_SET_REMOTE_AUTH(serv_cert, cert_len) do { \
        if (ott_set_remote_auth((serv_cert), (cert_len)) != PROTO_OK) \
		return false; \
} while(0)

#define PROTO_GET_POLLING() ott_get_polling_interval()
#define PROTO_SET_POLLING(new_value) ott_set_polling_interval((new_value));

#define PROTO_INITIATE_QUIT(send_nack) do { \
        ott_initiate_quit((send_nack)); \
} while(0)

#define PROTO_SEND_MSG_TO_CLOUD(msg, sz, svc_id, cb, data) do {		\
                (void)data; \
		if (ott_send_msg_to_cloud((msg), (sz), (svc_id), \
					  (cc_send_cb)) != PROTO_OK) {	\
		reset_conn_states(); \
		return CC_SEND_FAILED; \
	} \
} while(0)

#define PROTO_SEND_STATUS_MSG_TO_CLOUD(msg, sz, cb) do {		\
		if (ott_send_msg_to_cloud((msg), (sz), \
                        (CC_SERVICE_BASIC), (cc_send_cb)) != PROTO_OK) { \
		        reset_conn_states(); \
		        return CC_SEND_FAILED; \
                } \
} while(0)

#define PROTO_SEND_DIAG_MSG_TO_CLOUD(msg, sz, cb) do {		\
		if (ott_send_msg_to_cloud((msg), (sz), \
			(CC_SERVICE_BASIC), (cc_send_cb)) != PROTO_OK) { \
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

#define PROTO_SEND_MSG_TO_CLOUD(msg, sz, svc_id, cb, data) do {		\
                (void)data; \
		if (smsnas_send_msg_to_cloud((msg), (sz), (svc_id), \
					  (cc_send_cb)) != PROTO_OK) {	\
		reset_conn_states(); \
		return CC_SEND_FAILED; \
	} \
} while(0)

#define PROTO_SEND_STATUS_MSG_TO_CLOUD(msg, sz, cb) do {		\
		if (smsnas_send_msg_to_cloud((msg), (sz), \
                        (CC_SERVICE_BASIC), (cc_send_cb)) != PROTO_OK) { \
		        reset_conn_states(); \
		        return CC_SEND_FAILED; \
                } \
} while(0)

#define PROTO_SEND_DIAG_MSG_TO_CLOUD(msg, sz, cb) do {		\
		if (smsnas_send_msg_to_cloud((msg), (sz), \
                        (CC_SERVICE_BASIC), (cc_send_cb)) != PROTO_OK) { \
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

#define PROTO_SET_OWN_AUTH(d_id, d_id_sz, d_sec, d_sec_sz) do { \
        (void)(d_id); \
        (void)(d_id_sz); \
        (void)(d_sec); \
        (void)(d_sec_sz); \
} while(0)

#define PROTO_SET_REMOTE_AUTH(serv_cert, cert_len) do { \
        (void)(serv_cert); \
        (void)(cert_len); \
} while(0)

#define PROTO_SET_POLLING(new_value) (void)((new_value))

#elif defined (MQTT_PROTOCOL)

#define PROTO_INIT() do { \
        if (mqtt_protocol_init() != PROTO_OK) \
                return false; \
} while(0)

#define PROTO_GET_RCVD_MSG_PTR(msg) mqtt_get_rcv_buffer_ptr((msg))

#define PROTO_SET_DESTINATION(dest) do { \
        if (mqtt_set_destination((dest)) != PROTO_OK) \
		return false; \
} while(0)

#define PROTO_SET_OWN_AUTH(d_id, d_id_sz, d_sec, d_sec_sz) do { \
        if (mqtt_set_own_auth((d_id), (d_id_sz), (d_sec), (d_sec_sz)) != PROTO_OK) \
		return false; \
} while(0)

#define PROTO_SET_REMOTE_AUTH(serv_cert, cert_len) do { \
        if (mqtt_set_remote_auth((serv_cert), (cert_len)) != PROTO_OK) \
		return false; \
} while(0)

#define PROTO_GET_POLLING() mqtt_get_polling_interval()
#define PROTO_SET_POLLING(new_value) (void)((new_value))

#define PROTO_INITIATE_QUIT(send_nack)

#define PROTO_SEND_MSG_TO_CLOUD(msg, sz, svc_id, cb, topic) do {		\
		if (mqtt_send_msg_to_cloud((msg), (sz), (svc_id), \
					  (cc_send_cb), topic) != PROTO_OK) {	\
		         reset_conn_states(); \
		         return CC_SEND_FAILED; \
                 } \
} while(0)

#define PROTO_SEND_STATUS_MSG_TO_CLOUD(msg, sz, cb) do {		\
		if (mqtt_send_status_msg_to_cloud((msg), (sz), \
                        (cc_send_cb)) != PROTO_OK) { \
		        reset_conn_states(); \
		        return CC_SEND_FAILED; \
                } \
} while(0)

#define PROTO_SEND_DIAG_MSG_TO_CLOUD(msg, sz, cb) do {		\
		if (mqtt_send_diag_msg_to_cloud((msg), (sz), \
                        (cc_send_cb)) != PROTO_OK) { \
		        reset_conn_states(); \
		        return CC_SEND_FAILED; \
                } \
} while(0)

#define PROTO_SEND_ACK()
#define PROTO_SEND_NACK()

#define PROTO_SET_RECV_BUFFER_CB(rcv_buf, sz, rcv_cb) do { \
        if (mqtt_set_recv_buffer_cb((rcv_buf), (sz), (rcv_cb)) != PROTO_OK) \
                return CC_RECV_FAILED; \
} while(0)

#define PROTO_MAINTENANCE(poll_due, cur_ts) do { \
        (void)(poll_due); \
        mqtt_maintenance(cur_ts); \
} while(0)

#else
#error "define valid protocol options from OTT_PROTOCOL/SMSNAS_PROTOCOL/MQTT_PROTOCOL"
#endif /* OTT_PROTOCOL ifelse ends */

#endif
