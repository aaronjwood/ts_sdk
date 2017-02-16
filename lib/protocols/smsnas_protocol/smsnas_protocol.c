/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include "smsnas_protocol.h"
#include "smsnas_def.h"

#define INVOKE_SEND_CALLBACK(_buf, _sz, _evt) \
	do { \
		if (session.send_cb) \
			session.send_cb((_buf), (_sz), (_evt)); \
	} while(0)

#define INVOKE_RECV_CALLBACK(_buf, _sz, _evt) \
	do { \
		if (session.rcv_msg.cb) \
			session.rcv_msg.cb((_buf), (_sz), (_evt)); \
	} while(0)

#ifdef PROTO_TIME_PROFILE
static uint32_t proto_begin;
#endif

/* global counter for the concatenated message reference number to be used in
 * TP-USER header element
 */
static int msg_ref_num;

/* sleep interval, which can be set during control message sleep type */
static uint32_t sl_intr;

proto_result smsnas_protocol_init(void)
{
	session.send_buf = NULL;
	session.send_sz = 0;
	session.send_cb = NULL;
	session.host_valid = false;
	session.rcv_msg.rcv_path_valid = false;
	session.rcv_msg.conct_in_progress = false;
	msg_ref_num = 0;
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

const uint8_t *smsnas_get_rcv_buffer_ptr(const void *msg)
{
	if (!msg || !session.rcv_msg.rcv_path_valid)
		return NULL;

	const msg_t *ptr_to_msg = (const msg_t *)(msg);
	return (const uint8_t *)&(ptr_to_msg->payload);
}

/*
 * Length of the binary data. FIXME: revisit this
 */
uint32_t smsnas_get_rcvd_data_len(const void *msg)
{
	if (!msg || !session.rcv_msg.rcv_path_valid)
		return 0;
	msg_t *ptr_to_msg = (msg_t *)(msg);
	return ptr_to_msg->payload_sz;
}

proto_result smsnas_set_recv_buffer_cb(void *rcv_buf, proto_pl_sz sz,
					proto_callback rcv_cb)
{
	if (!rcv_buf || (sz > PROTO_MAX_MSG_SZ))
		return PROTO_INV_PARAM;
	/* This means receive path currently in progress */
	if (session.rcv_msg.rcv_path_valid)
		return PROTO_ERROR;

	session.rcv_msg.buf = rcv_buf;
	session.rcv_msg.cb = rcv_cb;
	session.rcv_msg.wr_idx = 0;
	session.rcv_msg.cur_seq = 0;
	session.rcv_msg.rem_sz = sz;
	session.rcv_msg.rcv_path_valid = true;
	at_set_rcv_cb(smsnas_rcv_cb);
	return PROTO_OK;
}

void smsnas_send_ack(void)
{
        at_send_ack();
}

void smsnas_send_nack(void)
{
        at_send_nack();
}

static bool process_smsnas_control_msg(smsnas_ctrl_msg *msg)
{
	if (msg->version != SMSNAS_CTRL_MSG_VER)
		return false;
	switch (msg->msg_type) {
	case SMSNAS_MSG_TYPE_SLEEP:
		sl_intr = msg->interval;
		break;
	default:
		return false;
	}
	return true;
}

static void rcv_path_cleanup(void)
{
	session.rcv_msg.cur_seq = 0;
	session.rcv_msg.rcv_path_valid = false;
	session.rcv_msg.conct_in_progress = false;
}

static bool check_validity(at_msg_t *msg_ptr, bool first_seg)
{
	/* check for storage capacity */
	if (session.rcv_msg.rem_sz < msg_ptr->len)
		return false;

	if (first_seg) {
		if (msg_ptr->seq_no != 1)
			return false;
		smsnas_msg_t *smsnas_msg = (smsnas_msg_t *)msg_ptr->buf;
		if (smsnas_msg->version != SMSNAS_VERSION)
			return false;
		return true;
	}
	/* check for out of order segment */
	if (msg_ptr->seq_no != (session.rcv_msg.cur_seq + 1))
		return false;

	/* check for reference number, device does not process multiple
	 * reference numbers at the same time, FIXME: check back this scenario
	 */
	if (session.rcv_msg.cref_num != msg_ptr->concat_ref_no)
		return false;

	return true;
}

static update_ack_rcv_path(at_msg_t *msg_ptr)
{
	session.rcv_msg.cur_seq = msg_ptr->seq_no;
	session.rcv_msg.rem_sz -= msg_ptr->len;
	session.rcv_msg.wr_idx += msg_ptr->len;
	smsnas_send_ack();
}

/* FIXME: revisit all the at layer function callings including this one */
static void smsnas_rcv_cb(at_msg_t *msg)
{
	if (msg_ptr->num_seg > 1) {
		if (session.rcv_msg.cur_seq == 0) {
			/* This is the start of the concatenated messages */
			if (!check_validity(msg_ptr, true))
				goto error;
			session.rcv_msg.wr_idx = 0;
			session.rcv_msg.cref_num = msg_ptr->concat_ref_no;
			memcpy(session.rcv_msg.buf + session.rcv_msg.wr_idx,
				msg_ptr->buf, msg_ptr->len);
			session.rcv_msg.conct_in_progress = true;
		} else {
			if (!check_validity(msg_ptr, false))
				goto error;
			memcpy(session.rcv_msg.buf + session.rcv_msg.wr_idx,
				msg_ptr->buf, msg_ptr->len);

			/* check for last segment */
			if (msg_ptr->seq_no == msg_ptr->num_seg) {
				/* Invoke upper level callback and let upper
				 * level ack this message
				 */
				INVOKE_RECV_CALLBACK(session.rcv_msg.buf,
					session.rcv_msg.wr_idx + msg_ptr->len,
					PROTO_RCVD_SMSNAS_MSG);
				goto done;

			}
		}
		update_ack_rcv_path(msg_ptr);
		return;

	} else {
		smsnas_msg_t *smsnas_msg = (smsnas_msg_t *)msg_ptr->buf;
		if (smsnas_msg->version != SMSNAS_VERSION)
			goto error;
		if (smsnas_msg->service_id == 0) {
			if (!process_smsnas_control_msg(smsnas_msg->data.bytes))
				goto error;
			smsnas_send_ack();
			goto done;
		}
		if (msg_ptr->len > session.rcv_msg.rem_sz) {
			INVOKE_RECV_CALLBACK(session.rcv_msg.buf, 0,
						PROTO_RCVD_SMSNAS_MEM_INSUF);
			goto error;
		}
		memcpy(session.rcv_msg.buf, msg_ptr->buf, msg_ptr->len);
		/* let upper level decide to ack/nack this message */
		INVOKE_RECV_CALLBACK(session.rcv_msg.buf, msg_ptr->len,
					PROTO_RCVD_SMSNAS_MSG);
		goto done;

	}
error:
	smsnas_send_nack();
done:
	rcv_path_cleanup();
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
		return PROTO_TIMEOUT;
	return PROTO_OK;
}

/* takes a payload and builds SMSNAS protocol message */
static void build_smsnas_msg(const void *payload, proto_pl_sz sz, uint8_t s_id)
{
	memset(session.send_msg, 0, MAX_SMS_PL_SZ);
	session.send_msg[0] = SMSNAS_VERSION;
	session.send_msg[1] = s_id;
	session.send_msg[2] = (uint8_t)(sz & 0xff);
	session.send_msg[3] = (uint8_t)((sz >> 8) & 0xff);
	memcpy((session.send_msg + PROTO_OVERHEAD_SZ), payload, sz);
}

/* Calculates number of segments for the given size when sz exceeds
 * MAX_SMS_PL_WITHOUT_HEADER bytes of payload
 */

int calculate_total_msgs(proto_pl_sz sz)
{
	bool first = true;
	int total_msg = 1;
	int max_size = MAX_SMS_SZ_WITH_HEADER;
	while (sz > 0) {
		if (first) {
			sz = sz - max_size;
			total_msg++;
			max_size = MAX_SMS_SZ_WITH_HD_WITHT_OVHD;
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

/* Prepares SMSNAS protocol message and sends it over to lower level
 * if sz exceeds MAX_SMS_PL_WITHOUT_HEADER bytes , it has to be sent as a
 * concatenated message
 */
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
	uint8_t *temp_buf = NULL;
	proto_pl_sz rem_sz = sz;
	proto_pl_sz send_sz = 0;
	proto_pl_sz total_sent = 0;
	while (1) {
		if (cur_seq_num == 1) {
			build_smsnas_msg(buf, MAX_SMS_SZ_WITH_HEADER,
					service_id);
			temp_buf = session.send_msg;
			rem_sz = rem_sz - MAX_SMS_SZ_WITH_HEADER;
			send_sz = MAX_SMS_SZ_WITH_HEADER + PROTO_OVERHEAD_SZ;
		}
		proto_result ret = write_to_modem(temp_buf, send_sz,
					msg_ref_num, total_msgs, cur_seq_num);
		if (ret != PROTO_OK)
			return ret;
		cur_seq_num++;
		if (cur_seq_num > total_msgs)
			break;
		total_sent += send_sz;
		temp_buf = buf + total_sent;
		if (rem_sz <= MAX_SMS_SZ_WITH_HD_WITHT_OVHD)
			send_sz = rem_sz;
		else {
			rem_sz = rem_sz - MAX_SMS_SZ_WITH_HD_WITHT_OVHD;
			send_sz = MAX_SMS_SZ_WITH_HD_WITHT_OVHD;
		}
	}
	msg_ref_num = (msg_ref_num + 1) % MAX_SMS_REF_NUMBER;
	return PROTO_OK;
}

/* Checks if it has started or in middle of receiving mnultiple segements
 * in which case, report back the time in miliseconds for upper level to
 * check back if protocol has received next segment
 */
uint32_t smsnas_maintenance(void)
{
	/* There is nothing to maintain if receive path is not expecting
	 * concatenated messages
	 */
	if (!session.rcv_msg.conct_in_progress)
		return 0;

	/* if condition evaluates to be true, smsnas_maintenance is being called
	 * out of timeout and it did not receive next segement, invoke receive
	 * callback with receive timeout event
	 */
	if (session.rcv_msg.expected_seq != session.rcv_msg.cur_seq) {
		INVOKE_RECV_CALLBACK(session.rcv_msg.buf, session.rcv_msg.wr_idx,
					PROTO_RCV_TIMEOUT);
		rcv_path_cleanup();
	}
	/* Check if it is the start of the receiving concatenated messages */
	if ((session.rcv_msg.cur_seq == 1) ||
		(session.rcv_msg.expected_seq == session.rcv_msg.cur_seq))
		goto done;

done:
	session.rcv_msg.expected_seq = session.rcv_msg.cur_seq + 1;
	return CONC_NEXT_SEG_TIMEOUT_MS;
}
