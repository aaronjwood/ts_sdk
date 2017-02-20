/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include "smsnas_protocol.h"
#include "smsnas_def.h"

#define INVOKE_RECV_CALLBACK(_buf, _sz, _evt, _s_id) \
	do { \
		if (session.rcv_cb) \
			session.rcv_cb((_buf), (_sz), (_evt), (_s_id)); \
	} while(0)

#ifdef PROTO_TIME_PROFILE
static uint32_t proto_begin;
#endif

/* sleep interval, which can be set during control message sleep type */
static uint32_t sl_intr;
static uint32_t current_polling_interval;

proto_result smsnas_protocol_init(void)
{
	sl_intr = 0;
	uint8_t i = 0;

	current_polling_interval = INIT_POLLING_MS;
	memset(session.host, 0, MAX_HOST_LEN);
	session.host_valid = false;

	uint8_t i;
	uint8_t num_rcv_path = ARRAY_SIZE(session.rcv_msg);

	for (i = 0; i < num_rcv_path; i++) {
		if ((i == 0) && (num_rcv_path == 1)) {
			session.rcv_msg[i].buf = NULL;
		else
			session.rcv_msg[i].buf = (smsnas_rcv_buf +
							(i * PROTO_MAX_MSG_SZ));
		session.rcv_msg[i].rem_sz = PROTO_MAX_MSG_SZ;
		session.rcv_msg[i].init_sz = PROTO_MAX_MSG_SZ;
		session.rcv_msg[i].wr_idx = i * PROTO_MAX_MSG_SZ;
		session.rcv_msg[i].cur_seq = 0;
		session.rcv_msg[i].rcv_path_valid = false;
		session.rcv_msg[i].conct_in_progress = false;
		session.rcv_msg[i].ack_nack_pend = NO_ACK_NACK;
	}

	session.rcv_path_num = -1;
	session.msg_ref_num = 0;
	session.rcv_cb = NULL;
	return PROTO_OK;
}

proto_result smsnas_set_destination(const char *host)
{
	if (!host)
		return PROTO_INV_PARAM;

	if (((strlen(host) + 1) > MAX_HOST_LEN) || (strlen(host) == 0))
		return PROTO_INV_PARAM;

	strncpy(session.host, host, strlen(host));
	session.host_valid = true;
	return PROTO_OK;
}

void smsnas_set_sleep_interval(uint32_t sleep_int)
{
	sl_intr = sleep;
}

uint32_t smsnas_get_sleep_interval(void)
{
	return sl_intr;
}

const uint8_t *smsnas_get_rcv_buffer_ptr(const void *msg)
{
	if (!msg || !session.rcv_cb)
		return NULL;

	const smsnas_msg_t *ptr_to_msg = (const smsnas_msg_t *)(msg);
	return (const uint8_t *)&(ptr_to_msg->payload);
}

/*
 * Length of the binary data
 */
uint32_t smsnas_get_rcvd_data_len(const void *msg)
{
	if (!msg || !session.rcv_cb)
		return 0;
	const smsnas_msg_t *ptr_to_msg = (const smsnas_msg_t *)(msg);
	return ptr_to_msg->payload_sz;
}

proto_result smsnas_set_recv_buffer_cb(void *rcv_buf, proto_pl_sz sz,
					proto_callback rcv_cb)
{
	if (!rcv_buf || (sz > PROTO_MAX_MSG_SZ))
		return PROTO_INV_PARAM;

	/* This means receive path currently in progress */
	if (session.rcv_cb)
		return PROTO_ERROR;

	if (ARRAY_SIZE(session.rcv_msg) == 1) {
		session.rcv_msg[0].buf = rcv_buf;
		session.rcv_msg[0].rem_sz = sz;
		session.rcv_msg[0].init_sz = sz;
		session.rcv_msg[0].rcv_path_valid = true;
	}
	/* place holder for the upper level buffer, used when multi receive path
	 * is present
	 */
	session.rcv_buf = rcv_buf;
	session.rcv_sz = sz;

	session.rcv_cb = rcv_cb;
	at_set_rcv_cb(smsnas_rcv_cb);
	return PROTO_OK;
}

static bool smsnas_send_ack_nack(ack_nack an)
{
	int rcv_path = session.rcv_path_num;
	if ((rcv_path == -1) || (rcv_path >= ARRAY_SIZE(session.rcv_msg)) ||
		(an > NACK_PENDING))
		RETURN_ERROR(false, "Invalid rcv_path or ack/nack state");

	session.rcv_msg[rcv_path].ack_nack_pend = an;
}

void smsnas_send_ack(void)
{
	smsnas_send_ack_nack(ACK_PENDING);
}

void smsnas_send_nack(void)
{
        smsnas_send_ack_nack(NACK_PENDING);
}

static void rcv_path_cleanup(void)
{
	current_polling_interval = 0;
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

static void smsnas_rcv_cb(at_msg_t *msg)
{
	if (!session.rcv_cb)
		return;
	if (msg_ptr->num_seg > 1) {
		if (session.rcv_msg.cur_seq == 0) {
			/* This is the start of the concatenated messages */
			if (!check_validity(msg_ptr, true))
				goto error;
			session.rcv_msg.wr_idx = 0;
			session.rcv_msg.cref_num = msg_ptr->concat_ref_no;
			memcpy(session.rcv_msg.buf, msg_ptr->buf, msg_ptr->len);
			smsnas_msg_t *temp_msg = session.rcv_msg.buf;
			session.rcv_msg.service_id = temp_msg->service_id;
			session.rcv_msg.conct_in_progress = true;
			current_polling_interval = CONC_NEXT_SEG_TIMEOUT_MS;
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
					PROTO_RCVD_SMSNAS_MSG,
					session.rcv_msg.service_id);
				goto done;

			}
		}
		update_ack_rcv_path(msg_ptr);
		return;

	} else {
		smsnas_msg_t *smsnas_msg = (smsnas_msg_t *)msg_ptr->buf;
		if (smsnas_msg->version != SMSNAS_VERSION)
			goto error;
		if (msg_ptr->len > session.rcv_msg.rem_sz) {
			INVOKE_RECV_CALLBACK(session.rcv_msg.buf, msg_ptr->len,
						PROTO_RCVD_SMSNAS_MEM_INSUF,
						smsnas_msg->service_id);
			goto error;
		}
		memcpy(session.rcv_msg.buf, msg_ptr->buf, msg_ptr->len);
		/* let upper level decide to ack/nack this message */
		INVOKE_RECV_CALLBACK(session.rcv_msg.buf, msg_ptr->len,
					PROTO_RCVD_SMSNAS_MSG,
					smsnas_msg->service_id);
		goto done;

	}
error:
	smsnas_send_nack();
done:
	rcv_path_cleanup();
}

static void handle_pend_ack_nack(void)
{
	switch (session.rcv_msg.ack_nack_pend) {
	case ACK_PENDING:
		at_send_ack();
		break;
	case NACK_PENDING:
		at_send_nack();
		break;
	default:
		break;
	}
	session.rcv_msg.ack_nack_pend = 0;
}

/* Checks if any pending ack or nack of previously received message
 * If poll_due is true, it was in the middle of receiving concatenated message
 * and checks if it received next segment during polling time, if not then
 * invoke callback indicating receiving timeout to upper level.
 */
void smsnas_maintenance(bool poll_due)
{

	handle_pend_ack_nack();

	if (!poll_due)
		return;
	/* There is nothing to maintain if receive path is not expecting
	 * concatenated messages
	 */
	if (!session.rcv_msg.conct_in_progress)
		return;

	/* Start of the concatenated segment and polling interval to check for
	 * arrival of next segment
	 */
	if (session.rcv_msg.cur_seq == 1)
		goto done;

	/* if condition to be true, smsnas_maintenance is being called
	 * out of timeout and it did not receive next segement meanwhile,
	 * invoke receive callback with receive timeout event
	 */
	if (session.rcv_msg.expected_seq != session.rcv_msg.cur_seq) {
		INVOKE_RECV_CALLBACK(session.rcv_msg.buf, session.rcv_msg.wr_idx,
					PROTO_RCV_TIMEOUT);
		rcv_path_cleanup();
		return;
	}
done:
	session.rcv_msg.expected_seq = session.rcv_msg.cur_seq + 1;
}

static proto_result write_to_modem(const uint8_t *msg, proto_pl_sz len,
			uint8_t ref_num, uint8_t total_num, uint8_t seq_num)
{
	uint8_t retry = 0;

	at_msg_t sm_msg;
	sm_msg.buf = msg;
	sm_msg.len = len;
	sm_msg.concat_ref_no = ref_num;
	sm_msg.num_seg = total_num;
	sm_msg.seq_no = seq_num;
	memset(sm_msg.addr, 0, ADDR_SZ);
	memcpy(sm_msg.addr, session.host, strlen(session.host));

	bool ret = at_send_sms(&sm_msg);
	while ((!ret) && (retry < MAX_RETRIES)) {
		ret = at_send_sms(&sm_msg);
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
	return total_msg;
}

static void invoke_send_callback(const void *buf, proto_pl_sz sz, uint8_t s_id,
				proto_callback cb, proto_result res)
{
	switch (res) {
	case PROTO_TIMEOUT:
		cb(buf, sz, PROTO_SEND_TIMEOUT, s_id);
		break;
	default:
		break;
	}
}

/* Prepares SMSNAS protocol message and sends it over to lower level
 * if sz exceeds MAX_SMS_PL_WITHOUT_HEADER bytes , it has to be sent as a
 * concatenated message
 */
proto_result smsnas_send_msg_to_cloud(const void *buf, proto_pl_sz sz,
					uint8_t service_id, proto_callback cb)
{
	if (!buf || (sz == 0) || (sz > PROTO_MAX_MSG_SZ))
		return PROTO_INV_PARAM;

	/* flag gets set during smsnas_set_destination API call */
	if (!session.host_valid)
		return PROTO_INV_PARAM;

	uint8_t msg_ref_num = session.msg_ref_num;
	uint8_t total_msgs = 0;
	uint8_t cur_seq_num = 0;
	/* check if it needs to be concatenated message */
	if (sz <= MAX_SMS_PL_WITHOUT_HEADER) {
		build_smsnas_msg(buf, sz, service_id);
		proto_result res = write_to_modem(session.send_msg,
					sz + PROTO_OVERHEAD_SZ, msg_ref_num,
					total_msgs, cur_seq_num);
		if (res != PROTO_OK) {
			invoke_send_callback(buf, sz, service_id, cb, res);
			return PROTO_ERROR;
		}

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
		if (ret != PROTO_OK) {
			invoke_send_callback(buf, sz, service_id, cb, res);
			return PROTO_ERROR;
		}
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
	session.msg_ref_num = (msg_ref_num + 1) % MAX_SMS_REF_NUMBER;
	return PROTO_OK;
}

uint32_t smsnas_get_polling_interval(void)
{
	return current_polling_interval;
}
