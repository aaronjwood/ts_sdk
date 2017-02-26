/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include "smsnas_protocol.h"
#include "smsnas_def.h"
#include "platform.h"

#define RETURN_ERROR(string, ret) \
	do { \
		PRINTF_ERR("%s:%d:" #string, __func__, __LINE__); \
		PRINTF_ERR("\n"); \
		return (ret); \
	} while (0)

#define IN_FUNCTION_AT() \
	do { \
		PRINTF_FUNC("%s:%d:\n", __func__, __LINE__);\
	} while (0)

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

static void reset_rcv_path(uint8_t rcv_path)
{
	session.rcv_msg[rcv_path].rcv_sz = PROTO_MAX_MSG_SZ;
	session.rcv_msg[rcv_path].wr_idx = rcv_path * PROTO_MAX_MSG_SZ;
	session.rcv_msg[rcv_path].cur_seq = 0;
	session.rcv_msg[rcv_path].cref_num = -1;
	session.rcv_msg[rcv_path].conct_in_progress = false;
}

/* Initializes receive paths and resets internal protocol state */
proto_result smsnas_protocol_init(void)
{
	sl_intr = 0;
	uint8_t i = 0;

	/* This will not start until arrival of the first segement of the
	 * concatenated sms
	 */
	session.cur_polling_interval = 0;

	memset(session.host, 0, MAX_HOST_LEN);
	session.host_valid = false;

	uint8_t i;
	uint8_t num_rcv_path = ARRAY_SIZE(session.rcv_msg);

	for (i = 0; i < num_rcv_path; i++) {
		if ((i == 0) && (num_rcv_path == 1)) {
			/* buf will be used from upper level */
			session.rcv_msg[i].buf = NULL;
		else
			session.rcv_msg[i].buf = (smsnas_rcv_buf +
							(i * PROTO_MAX_MSG_SZ));
		reset_rcv_path(i);
	}

	session.msg_ref_num = 0;
	session.rcv_cb = NULL;
	session.rcv_valid = false;
	session.ack_nack_pend = NO_ACK_NACK;
	return PROTO_OK;
}

proto_result smsnas_set_destination(const char *host)
{
	if (!host)
		RETURN_ERROR("Invalid parameter", PROTO_INV_PARAM);

	if (((strlen(host) + 1) > MAX_HOST_LEN) || (strlen(host) == 0))
		RETURN_ERROR("Invalid parameter", PROTO_INV_PARAM);

	strncpy(session.host, host, strlen(host));
	session.host_valid = true;
	return PROTO_OK;
}

void smsnas_set_sleep_interval(uint32_t sleep_int)
{
	sl_intr = sleep_int;
}

uint32_t smsnas_get_sleep_interval(void)
{
	return sl_intr;
}

const uint8_t *smsnas_get_rcv_buffer_ptr(const void *msg)
{
	if (!msg)
		RETURN_ERROR("Invalid parameter", NULL);

	const smsnas_msg_t *ptr_to_msg = (const smsnas_msg_t *)(msg);
	return (const uint8_t *)&(ptr_to_msg->payload);
}

/*
 * Length of the binary data
 */
uint32_t smsnas_get_rcvd_data_len(const void *msg)
{
	if (!msg)
		RETURN_ERROR("Invalid parameter", 0);
	const smsnas_msg_t *ptr_to_msg = (const smsnas_msg_t *)(msg);
	return ptr_to_msg->payload_sz;
}

proto_result smsnas_set_recv_buffer_cb(void *rcv_buf, proto_pl_sz sz,
					proto_callback rcv_cb)
{
	if (!rcv_buf || (sz > PROTO_MAX_MSG_SZ))
		RETURN_ERROR("Invalid parameter", PROTO_INV_PARAM);

	/* This means receive path currently in progress */
	if (session.rcv_valid)
		RETURN_ERROR("receiver already in progress", PROTO_ERROR);

	/* If only single receive path is present, we will need to initialize
	 * here so that concatenated state machine is ready to use when need
	 * arise
	 */
	if (ARRAY_SIZE(session.rcv_msg) == 1) {
		session.rcv_msg[0].buf = rcv_buf;
		session.rcv_msg[0].rcv_sz = sz;
		session.rcv_msg[0].wr_idx = 0;
	}
	/* place holder for the upper level buffer, used when multi receive path
	 * are present
	 */
	session.rcv_buf = rcv_buf;
	session.rcv_sz = sz;
	session.rcv_valid = true;
	session.rcv_cb = rcv_cb;
	at_set_rcv_cb(smsnas_rcv_cb);
	return PROTO_OK;
}

void smsnas_send_ack(void)
{
	session.ack_nack_pend = ACK_PENDING;
}

void smsnas_send_nack(void)
{
        session.ack_nack_pend = NACK_PENDING;
}

static bool is_conct_in_progress(void)
{
	bool found = false;
	uint8_t i;
	for (i = 0; i < ARRAY_SIZE(session.rcv_msg); i++) {
		if (session.rcv_msg[i].conct_in_progress) {
			found = true;
			break;
		}
	}
	return found;
}

static void rcv_path_cleanup(int rcv_path)
{
	if (!is_conct_in_progress())
		session.cur_polling_interval = 0;
	if (rcv_path == -1)
		return;
	reset_rcv_path(rcv_path);
}

static bool check_mem_overflow(proto_pl_sz rcv_len, proto_pl_sz pl_sz,
				uint8_t s_id)
{
	/* check for storage capacity */
	if (pl_sz < rcv_len) {
		INVOKE_RECV_CALLBACK(session.rcv_buf, session.rcv_sz,
				PROTO_RCVD_MEM_OVRFL, s_id);
		return true;
	}
	return false;
}

static bool check_validity(at_msg_t *msg_ptr, uint8_t rcv_path, bool first_seg)
{
	/* check if total number of segements are greater then MAX_CONC_SMS_NUM
	 */
	if (msg_ptr->num_seg > MAX_CONC_SMS_NUM)
		return false;

	if (first_seg) {
		smsnas_msg_t *smsnas_msg = (smsnas_msg_t *)msg_ptr->buf;
		if (msg_ptr->seq_no != 1)
			return false;
		if (smsnas_msg->version != SMSNAS_VERSION)
			return false;
		return true;
	}

	/* check for out of order segment */
	if (msg_ptr->seq_no != (session.rcv_msg[rcv_path].cur_seq + 1))
		return false;
	return true;
}

static update_ack_rcv_path(at_msg_t *msg_ptr, uint8_t rcv_path)
{
	session.rcv_msg[rcv_path].cur_seq = msg_ptr->seq_no;
	session.rcv_msg[rcv_path].wr_idx += msg_ptr->len;
	session.rcv_msg[rcv_path].next_seq_timeout = platform_get_tick_ms() +
							CONC_NEXT_SEG_TIMEOUT_MS;
	smsnas_send_ack();
}

/* detect or select receive path for the given msg if it is new msg check for
 * its sanity before allocating receive path
 */
static int retrieve_rcv_path(at_msg_t *msg, bool *new)
{
	uint8_t i;
	int rcvp = -1;
	int new_rcvp = -1;
	uint32_t timestamp = 0;
	uint8_t cref = msg->concat_ref_no;

	for (i = 0; i < ARRAY_SIZE(session.rcv_msg); i++) {
		if (session.rcv_msg[i].conct_in_progress) {
			if (i == 0)
				timestamp = session.rcv_msg[i].init_timestamp;
			/* middle of receiving concatenated sms */
			if (session.rcv_msg[i].cref_num == cref) {
				*new = false;
				return i;
			}
			if (session.rcv_msg[i].init_timestamp <= timestamp) {
				rcvp = i;
				timestamp = session.rcv_msg[i].init_timestamp;
			}
		} else
			new_rcvp = i;

	}
	/* Means new_rcvp receive path is free to use, else rcvp is the oldest
	 * concatenated sms to be evicted
	 */
	*new = true;
	if (new_rcvp != -1)
		rcvp = new_rcvp;

	/* check validity of the new message before allocating receive path */
	if (!check_validity(msg, rcvp, true))
		return -1;

	smsnas_msg_t *temp_msg = (smsnas_msg_t *)msg->buf;
 	session.rcv_msg[rcvp].service_id = temp_msg->service_id;

	return rcvp;
}

static void smsnas_rcv_cb(at_msg_t *msg)
{
	int rcv_path  = -1;
	proto_pl_sz rcvd = 0;
	/* Must be some random message that upper level is not expecting,
	 * ignore it and send nack
	 */
	if (!session.rcv_valid) {
		printf("%s:%d: rcv buffer not set\n", __func__, __LINE__);
		goto error;
	}

	if (msg_ptr->num_seg > 1) {
		bool new = false;
		rcv_path = retrieve_rcv_path(msg, &new);
		/* only possible when allocating new receive path */
		if (rcv_path == -1) {
			printf("%s: %d: Message is not valid\n",
				__func__, __LINE__);
			goto error;
		}
		smsnas_rcv_path *rp = &session.rcv_msg[rcv_path];
		rcvd = rp->wr_idx + msg_ptr->len;

		/* two step memory overflow check needed when there are multiple
		 * receive paths are supported, one for intermediate buffer
		 * and other for user supplied buffer
		 */
		if (check_mem_overflow(rcvd, session.rcv_sz, rp->service_id)) {
			session.rcv_valid = false;
			goto done;
		} else if ((ARRAY_SIZE(session.rcv_msg) > 1) &&
			(rcvd > rp->rcv_sz)) {
			printf("%s:%d: Intermediate buffer overflow\n",
				__func__, __LINE__);
			goto error;
		}
		if (new) {
			/* wr_idx is already been initialized */
			rp->cref_num = msg_ptr->concat_ref_no;
			memcpy(rp->buf, msg_ptr->buf, msg_ptr->len);
			rp->conct_in_progress = true;
			/* need to determine oldest to evict */
			rp->init_timestamp = platform_get_tick_ms();
		} else {
			if (!check_validity(msg_ptr, rcv_path, false))
				goto error;
			memcpy(rp->buf + rp->wr_idx, msg_ptr->buf, msg_ptr->len);

			/* check for last segment */
			if (msg_ptr->seq_no == msg_ptr->num_seg) {
				session.rcv_valid = false;

				/* Invoke upper level callback and let upper
				 * level ack/nack this message, also it is upper
				 * level's responsibility to schedule receive
				 * buffer hence rcv_valid is false here to catch
				 * that scenario where app services misbehaves
				 */

				/* for the case where only single receive path
				 * exists, no need to copy into rcv_buf as it
				 * is already being set as the only receiving
				 * buffer
				 */
				if (ARRAY_SIZE(sesison.rcv_msg) > 1)
					memcpy(session.rcv_buf, rp->buf, rcvd);

				INVOKE_RECV_CALLBACK(session.rcv_buf, rcvd,
					PROTO_RCVD_MSG, rp->service_id);
				goto done;
			}
		}
		update_ack_rcv_path(msg_ptr, rcv_path);
		return;
	} else {
		rcv_path = -1;
		smsnas_msg_t *smsnas_msg = (smsnas_msg_t *)msg_ptr->buf;
		if (smsnas_msg->version != SMSNAS_VERSION)
			goto error;
		session.rcv_valid = false;
		if (check_mem_overflow(msg_ptr->len, session.rcv_sz,
			smsnas_msg->service_id))
			goto done;
		memcpy(session.rcv_buf, msg_ptr->buf, msg_ptr->len);

		/* let upper level decide to ack/nack this message */
		INVOKE_RECV_CALLBACK(session.rcv_buf, msg_ptr->len,
					PROTO_RCVD_MSG, smsnas_msg->service_id);
		goto done;

	}
error:
	smsnas_send_nack();
done:
	rcv_path_cleanup(rcv_path);
}

static void handle_pend_ack_nack(void)
{
	switch (session.ack_nack_pend) {
	case ACK_PENDING:
		at_send_ack();
		break;
	case NACK_PENDING:
		at_send_nack();
		break;
	default:
		break;
	}
	session.ack_nack_pend = NO_ACK_NACK;
}

/* FIXME: verify the logic */
/* Checks if any pending ack or nack of previously received message
 * If poll_due is true, it was in the middle of receiving concatenated message
 * and checks if it received next segment during polling time, if not then
 * invoke callback indicating receiving timeout to upper level.
 */
void smsnas_maintenance(uint32_t cur_timestamp)
{

	handle_pend_ack_nack();

	/* There is nothing to maintain if receive paths are not expecting
	 * concatenated messages
	 */
	if (!is_conct_in_progress())
		return;

	uint8_t i;
	int rcvp = -1;
	uint32_t ns_time = 0;
	uint32_t *cur_int = &session.cur_polling_interval;

	for (i = 0; i < ARRAY_SIZE(session.rcv_msg); i++) {
		if (session.rcv_msg[i].conct_in_progress) {
			ns_time = session.rcv_msg[i].next_seq_timeout;

			if (cur_timestamp >=
				session.rcv_msg[i].next_seq_timeout) {
				if (session.rcv_msg[i].expected_seq !=
					session.rcv_msg[i].cur_seq) {
					INVOKE_RECV_CALLBACK(
						session.rcv_buf,
						session.rcv_sz,
						PROTO_RCV_TIMEOUT,
						session.rcv_msg[i].service_id);
					session.rcv_msg[i].conct_in_progress =
									false;
					rcv_path_cleanup(i);
					rcvp = i;
				}
			} else {
				if (*cur_int == 0)
					*cur_int = ns_time - cur_timestamp;
				else {
					if (*cur_int > (ns_time - cur_timestamp))
						*cur_int = ns_time -
								cur_timestamp;
				}
				if (session.rcv_msg[i].expected_seq !=
					(session.rcv_msg[i].cur_seq + 1))
					session.rcv_msg[i].expected_seq =
						session.rcv_msg[i].cur_seq + 1;
			}
		}
	}

	return session.cur_polling_interval;
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
		RETURN_ERROR("Retries exausted", PROTO_TIMEOUT);
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
		RETURN_ERROR("Invalid parameter", PROTO_INV_PARAM);

	/* flag gets set during smsnas_set_destination API call */
	if (!session.host_valid)
		RETURN_ERROR("Invalid parameter", PROTO_INV_PARAM);

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
			RETURN_ERROR("Send failed", PROTO_ERROR);
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
			RETURN_ERROR("Concatenated send failed", PROTO_ERROR);
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

/* If conatenated message is in progress, returns timeout to poll for the
 * next segment
 */
uint32_t smsnas_get_polling_interval(void)
{
	return session.cur_polling_interval;
}
