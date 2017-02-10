/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>
#include "dbg.h"
#include "platform.h"
#include "smsnas_protocol.h"
#include "smsnas_def.h"

#define INVOKE_SEND_CALLBACK(_buf, _sz, _evt) \
	do { \
		if (session.send_cb) \
			session.send_cb((_buf), (_sz), (_evt)); \
	} while(0)

#define INVOKE_RECV_CALLBACK(_buf, _sz, _evt) \
	do { \
		if (session.rcv_cb) \
			session.rcv_cb((_buf), (_sz), (_evt)); \
	} while(0)

#ifdef PROTO_TIME_PROFILE
static uint32_t proto_begin;
#endif

static int msg_ref_num;

static void smsnas_reset_state(void)
{
	session.send_buf = NULL;
	session.send_sz = 0;
	session.send_cb = NULL;
	session.host_valid = false;
	msg_ref_num = 0;
}

proto_result smsnas_protocol_init(void)
{
	smsnas_reset_state();
	return PROTO_OK;
}

proto_result smsnas_set_destination(const char *host)
{
	if (!host)
		return PROTO_INV_PARAM;

	if ((strlen(host) > MAX_HOST_LEN) || (strlen(host) == 0))
		return PROTO_INV_PARAM;

	strncpy(session.host, host, sizeof(session.host));
	session.host_valid = true;
	return PROTO_OK;
}

proto_result smsnas_set_recv_buffer_cb(void *rcv_buf, proto_pl_sz sz,
					proto_callback rcv_cb)
{
	if (!rcv_buf || (sz > PROTO_MAX_MSG_SZ))
		return PROTO_INV_PARAM;
	session.rcv_buf = rcv_buf;
	session.rcv_sz = sz;
	session.rcv_cb = rcv_cb;
	return PROTO_OK;
}


static proto_result write_to_modem(const uint8_t *msg, proto_pl_sz len,
				int ref_num, int total_num, int seq_num)
{
	uint8_t retry = 0;
	int ret = at_send_sms(msg, len, ref_num, total_num, seq_num);
	while ((ret < 0) && (retry < MAX_RETRIES)) {
		ret = at_send_sms(msg, len, ref_num, total_num, seq_num);
		retry++;
	}
	if (retry >= MAX_RETRIES)
		return PROTO_ERROR;
	return PROTO_OK;
}

/*
 * Length of the binary data.
 */
uint32_t smsnas_get_rcvd_data_len(const void *msg)
{
	if (!msg)
		return 0;
	msg_t *ptr_to_msg = (msg_t *)(msg);
	return ptr_to_msg->payload_sz;
}

/*
 * Process a received response. Most of the actual processing is done through the
 * user provided callbacks this function invokes. In addition, it sets some
 * internal flags to decide the state of the session in the future. Calling the
 * send callback is optional and is decided through "invoke_send_cb".
 * On receiving a NACK return "false". Otherwise, return "true".
 */
static bool process_recvd_msg(msg_t *msg_ptr, uint32_t rcvd, bool invoke_send_cb)
{
	c_flags_t c_flags;
	m_type_t m_type;
	bool no_nack_detected = true;
	OTT_LOAD_FLAGS(msg_ptr->cmd_byte, c_flags);
	OTT_LOAD_MTYPE(msg_ptr->cmd_byte, m_type);

	/* Keep the session alive if the cloud has more messages to send */
	session.pend_bit = OTT_FLAG_IS_SET(c_flags, CF_PENDING);

	if (OTT_FLAG_IS_SET(c_flags, CF_ACK)) {
		proto_event evt = PROTO_RCVD_NONE;
		/* Messages with a body need to be ACKed in the future */
		session.pend_ack = false;
		if (m_type == MT_UPDATE) {
			evt = PROTO_RCVD_UPD;
		} else if (m_type == MT_CMD_SL) {
			evt = PROTO_RCVD_CMD_SL;
		} else if (m_type == MT_CMD_PI) {
			INVOKE_RECV_CALLBACK(msg_ptr, rcvd, PROTO_RCVD_CMD_PI);
			session.pend_ack = true;
		}

		if (m_type == MT_UPDATE || m_type == MT_CMD_SL) {
			INVOKE_RECV_CALLBACK(msg_ptr, rcvd, evt);
		}
		if (invoke_send_cb) {
			INVOKE_SEND_CALLBACK(session.send_buf, session.send_sz,
						PROTO_RCVD_ACK);
		}
	} else if (OTT_FLAG_IS_SET(c_flags, CF_NACK)) {
		no_nack_detected = false;
		if (invoke_send_cb) {
			INVOKE_SEND_CALLBACK(session.send_buf, session.send_sz,
				PROTO_RCVD_NACK);
		}
	}

	if (OTT_FLAG_IS_SET(c_flags, CF_QUIT)) {
		ott_close_connection();
		ott_reset_state();
		INVOKE_RECV_CALLBACK(msg_ptr, rcvd, PROTO_RCVD_QUIT);
	}

	return no_nack_detected;
}

static void build_smsnas_msg(const void *payload, proto_pl_sz sz, uint8_t s_id)
{
	session.send_msg[0] = VERSION_BYTE;
	session.send_msg[1] = s_id;
	session.send_msg[2] = (uint8_t)(sz & 0xff);
	session.send_msg[3] = (uint8_t)((sz >> 8) & 0xff);
	memcpy((session.send_msg + PROTO_OVERHEAD_SZ), payload, sz);
}

int calculate_total_msgs(proto_pl_sz sz)
{
	bool first = true;
	int total_msg = 1;
	int max_size = MAX_SMS_SZ_WITH_HEADER
	while (sz > 0) {
		if (first) {
			sz = sz - max_size;
			total_msg++;
			max_size = MAX_SMS_SZ_WITH_HEADER + PROTO_OVERHEAD_SZ;
			first = false;
			continue;
		}
		if (sz <= max_size)
			return total_msg;
		else {
			total_msg++;
			sz = sz - max_size;
		}
	}
}

proto_result smsnas_send_msg_to_cloud(const void *buf, proto_pl_sz sz,
					uint8_t service_id)
{
	if (!buf || (sz == 0) || (sz > PROTO_MAX_MSG_SZ))
		return PROTO_INV_PARAM;

	/* flag gets set during smsnas_set_destination API call */
	if (!session.host_valid)
		return PROTO_INV_PARAM;

	int total_msgs = -1;
	int cur_seq_num = -1;
	if (sz <= MAX_SMS_PL_WITHOUT_HEADER) {
		build_smsnas_msg(buf, sz, service_id);
		return write_to_modem(session.send_msg, sz + PROTO_OVERHEAD_SZ,
					-1, total_msgs, cur_seq_num);
	}
	cur_seq_num = 1;
	total_msgs = calculate_total_msgs(sz);
	const void *temp_buf = NULL;
	proto_pl_sz cur_sz = sz;
	do {
		if (cur_seq_num == 1) {
			build_smsnas_msg(buf, MAX_SMS_SZ_WITH_HEADER,
					service_id);
			temp_buf = session.send_msg;
		} else {
			temp_buf = buf;
		}
		proto_result ret = write_to_modem(temp_buf, MAX_SMS_PL_SZ,
					msg_ref_num, total_msgs, cur_seq_num);

		if (ret != PROTO_OK)
			return ret;
	} while (cur_sz > 0);

	return PROTO_OK;
}

const uint8_t *smsnas_get_rcv_buffer_ptr(const void *msg)
{
	if (!msg)
		return NULL;

	m_type_t m_type;
	const msg_t *ptr_to_msg = (const msg_t *)(msg);
	return (const uint8_t *)&(ptr_to_msg->payload);

}

void smsnas_send_ack(void)
{
        at_send_ack();
}

void smsnas_send_nack(void)
{
        at_send_nack();
}
