/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include "smsnas_protocol.h"
#include "smsnas_def.h"
#include "platform.h"
#include "dbg.h"

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

/* Define intermediate buffer to store incoming messages */
static uint8_t smsnas_rcv_buf[PROTO_MAX_MSG_SZ * SMSNAS_MAX_RCV_PATH];

static void reset_rcv_path(uint8_t rcv_path)
{
	session.rcv_msg[rcv_path].rcv_sz = PROTO_MAX_MSG_SZ;
	session.rcv_msg[rcv_path].wr_idx = rcv_path * PROTO_MAX_MSG_SZ;
	session.rcv_msg[rcv_path].cur_seq = 0;
	session.rcv_msg[rcv_path].cref_num = -1;
	session.rcv_msg[rcv_path].conct_in_progress = false;
	session.rcv_msg[rcv_path].expected_seq = 0;
}

/* Initializes receive paths and resets internal protocol state */
proto_result smsnas_protocol_init(void)
{
	uint8_t i = 0;
	if (!at_init()) {
		RETURN_ERROR("modem init failed", PROTO_ERROR);
	}
	/* This will not be set until arrival of the first segement of the
	 * concatenated sms
	 */
	session.next_seg_rcv_timeout = 0;

	memset(session.host, 0, MAX_HOST_LEN);
	session.host_valid = false;

	for (i = 0; i < ARRAY_SIZE(session.rcv_msg); i++) {
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

	if ((strlen(host) > MAX_HOST_LEN) || (strlen(host) == 0))
		RETURN_ERROR("Invalid parameter", PROTO_INV_PARAM);

	strncpy(session.host, host, strlen(host));
	session.host_valid = true;
	return PROTO_OK;
}

const uint8_t *smsnas_get_rcv_buffer_ptr(const void *msg)
{
	if (!msg)
		RETURN_ERROR("Invalid parameter", NULL);

	const smsnas_msg_t *ptr_to_msg = (const smsnas_msg_t *)(msg);
	return ptr_to_msg->payload;
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
		session.next_seg_rcv_timeout = 0;
	if (rcv_path == -1)
		return;
	reset_rcv_path(rcv_path);
}

static bool check_mem_overflow(proto_pl_sz rcv_len, proto_pl_sz pl_sz,
				proto_service_id s_id)
{
	/* check for storage capacity */
	if (pl_sz < rcv_len) {
		INVOKE_RECV_CALLBACK(session.rcv_buf, session.rcv_sz,
				PROTO_RCVD_MEM_OVRFL, s_id);
		return true;
	}
	return false;
}

static bool check_validity(const at_msg_t *msg_ptr, uint8_t rcv_path,
			bool first_seg)
{
	/* check if total number of segements are greater then MAX_CONC_SMS_NUM
	 */
	if (msg_ptr->num_seg > MAX_CONC_SMS_NUM)
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
	/* FIXME: action to perform for duplicate message, currently it will
	 * nack and discard whole concatenated message for any out of sequence
	 * sms
	 */
	if (msg_ptr->seq_no != (session.rcv_msg[rcv_path].cur_seq + 1))
		return false;
	return true;
}

static void update_ack_rcv_path(const at_msg_t *msg_ptr, uint8_t rcv_path)
{
	session.rcv_msg[rcv_path].cur_seq = msg_ptr->seq_no;
	session.rcv_msg[rcv_path].wr_idx += msg_ptr->len;
	session.rcv_msg[rcv_path].next_seq_timeout = platform_get_tick_ms() +
							CONC_NEXT_SEG_TIMEOUT_MS;
	smsnas_send_ack();
}

/* Select receive path for the given msg if it is new msg check for
 * its sanity before allocating receive path
 */
static int retrieve_rcv_path(const at_msg_t *msg, bool *new,
				proto_service_id *s_id)
{
	uint8_t i;
	int new_rcvp = -1;
	for (i = 0; i < ARRAY_SIZE(session.rcv_msg); i++) {
		if (session.rcv_msg[i].conct_in_progress) {
			/* middle of receiving concatenated sms */
			if (session.rcv_msg[i].cref_num == msg->ref_no) {
				*new = false;
				return i;
			}
		} else
			new_rcvp = i;

	}
	if (new_rcvp == -1)
		return -1;
	/* Means new_rcvp receive path is free to use
	 */
	*new = true;
	/* check validity of the new message before allocating receive path */
	if (!check_validity(msg, new_rcvp, true))
		return -1;

	smsnas_msg_t *temp_msg = (smsnas_msg_t *)msg->buf;
	*s_id = temp_msg->service_id;
	return new_rcvp;
}

static void smsnas_rcv_cb(const at_msg_t *msg_ptr)
{
	int rcv_path  = -1;
	proto_pl_sz rcvd = 0;
	proto_service_id s_id = 0;
	/* Must be some random message that upper level is not expecting,
	 * ignore it and send nack
	 */
	if (!session.rcv_valid) {
		printf("%s:%d: rcv buffer not set\n", __func__, __LINE__);
		goto error;
	}

	if (msg_ptr->num_seg > 1) {
		IN_FUNCTION_AT();
		bool new = false;
		rcv_path = retrieve_rcv_path(msg_ptr, &new, &s_id);
		/* only possible when allocating new receive path */
		if (rcv_path == -1) {
			printf("%s: %d: Message is not valid\n",
				__func__, __LINE__);
			goto error;
		}
		smsnas_rcv_path *rp = &session.rcv_msg[rcv_path];
		rcvd = rp->wr_idx + msg_ptr->len;
		IN_FUNCTION_AT();
		/* two step memory overflow check, one for intermediate buffer
		 * and other for user supplied buffer
		 */
		if (check_mem_overflow(rcvd, session.rcv_sz, s_id)) {
			/* let upper level nack this sms first and then
			 * reschedule receive buffer
			 */
			IN_FUNCTION_AT();
			session.rcv_valid = false;
			goto done;
		} else if (rcvd > rp->rcv_sz) {
			printf("%s:%d: Unlikely Intermediate buffer overflow\n",
				__func__, __LINE__);

			/* FIXME: Current behaviour is to nack this message
			 * and return resetting receive path
			 */
			goto error;
		}
		if (new) {
			IN_FUNCTION_AT();
			rp->service_id = s_id;
			rp->cref_num = msg_ptr->ref_no;
			memcpy(rp->buf, msg_ptr->buf, msg_ptr->len);
			rp->conct_in_progress = true;
		} else {
			IN_FUNCTION_AT();
			if (!check_validity(msg_ptr, rcv_path, false))
				goto error;
			memcpy(rp->buf + rp->wr_idx, msg_ptr->buf, msg_ptr->len);
			IN_FUNCTION_AT();
			/* check for last segment */
			if (msg_ptr->seq_no == msg_ptr->num_seg) {
				session.rcv_valid = false;
				/* Invoke upper level callback and let upper
				 * level ack/nack this message, also it is upper
				 * level's responsibility to schedule receive
				 * buffer hence rcv_valid is false here to catch
				 * that scenario where app/services misbehave
				 */

				memcpy(session.rcv_buf, rp->buf, rcvd);
				dbg_printf("Received Update Message of bytes: %u\n", rcvd);
				for (uint16_t i = 0; i < rcvd; i++)
					dbg_printf("\t\t\t\t[Byte %u]: 0x%x, ", i, rp->buf[i]);
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
		/* Upper level has to activate receive buffer again */
		session.rcv_valid = false;
		if (check_mem_overflow(msg_ptr->len, session.rcv_sz,
			smsnas_msg->service_id)) {
			IN_FUNCTION_AT();
			goto done;
		}
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

proto_result smsnas_set_recv_buffer_cb(void *rcv_buf, proto_pl_sz sz,
					proto_callback rcv_cb)
{
	if (!rcv_buf || (sz > PROTO_MAX_MSG_SZ))
		RETURN_ERROR("Invalid parameter", PROTO_INV_PARAM);

	/* This means receive path currently in progress */
	if (session.rcv_valid)
		RETURN_ERROR("receiver already in progress", PROTO_ERROR);

	session.rcv_buf = rcv_buf;
	session.rcv_sz = sz;
	session.rcv_valid = true;
	session.rcv_cb = rcv_cb;
	at_sms_set_rcv_cb(smsnas_rcv_cb);
	return PROTO_OK;
}

/* FIXME: handle scenario where ack/nack function fails */
static void handle_pend_ack_nack(void)
{
	ack_nack an = session.ack_nack_pend;
	session.ack_nack_pend = NO_ACK_NACK;
	switch (an) {
	case ACK_PENDING:
		if(!at_sms_ack())
			printf("Ack failed\n");
		break;
	case NACK_PENDING:
		if(!at_sms_nack())
			printf("Nack failed\n");
		break;
	default:
		break;
	}
}

/* Checks if any pending ack or nack of previously received message
 * If poll_due is true, it was in the middle of receiving concatenated message
 * and checks if it received next segment during polling time, if not then
 * invoke callback indicating receiving timeout to upper level, also adjust
 * remaining timeouts for other receiving paths
 */
void smsnas_maintenance(bool poll_due, uint64_t cur_timestamp)
{

	handle_pend_ack_nack();

	/* There is nothing to maintain if receive paths are not expecting
	 * concatenated messages
	 */
	if (!is_conct_in_progress())
		return;

	uint8_t i;
	uint32_t ns_time = 0;
	uint64_t *cur_int = &session.next_seg_rcv_timeout;
	bool init_poll = false;

	for (i = 0; i < ARRAY_SIZE(session.rcv_msg); i++) {
		if (session.rcv_msg[i].conct_in_progress) {
			if (cur_timestamp >=
				session.rcv_msg[i].next_seq_timeout) {
				if (session.rcv_msg[i].cur_seq <
					session.rcv_msg[i].expected_seq) {
					/* Discard whole concatenated sms */
					session.rcv_msg[i].conct_in_progress =
									false;
					rcv_path_cleanup(i);
				}
			} else {
				ns_time = session.rcv_msg[i].next_seq_timeout;
				if (session.rcv_msg[i].expected_seq !=
					(session.rcv_msg[i].cur_seq + 1))
					session.rcv_msg[i].expected_seq =
						session.rcv_msg[i].cur_seq + 1;
				if (poll_due)
					ns_time -= cur_timestamp;
				session.rcv_msg[i].next_seq_timeout = ns_time;
				if (!init_poll) {
					*cur_int = ns_time;
					init_poll = true;
				}
				/* find the minimum */
				if (*cur_int >= ns_time)
					*cur_int = ns_time;
			}
		}
	}
}

/* FIXME: handle scenario where message is received when in middle of sending sms
 */
static proto_result write_to_modem(const uint8_t *msg, proto_pl_sz len,
			uint8_t ref_num, uint8_t total_num, uint8_t seq_num)
{
	uint8_t retry = 0;

	at_msg_t sm_msg;
	sm_msg.buf = (uint8_t *)msg;
	sm_msg.len = len;
	sm_msg.ref_no = ref_num;
	sm_msg.num_seg = total_num;
	sm_msg.seq_no = seq_num;
	sm_msg.addr = session.host;
	printf("%s:%d: size to send:%u\n", __func__, __LINE__, sm_msg.len);
	bool ret = at_sms_send(&sm_msg);
	while ((!ret) && (retry < MAX_RETRIES)) {
		ret = at_sms_send(&sm_msg);
		retry++;
	}
	if (retry > MAX_RETRIES)
		RETURN_ERROR("Retries exausted", PROTO_TIMEOUT);
	return PROTO_OK;
}

/* takes a payload and builds SMSNAS protocol message */
static void build_smsnas_msg(const void *payload, proto_pl_sz total_sz,
				proto_pl_sz cp_sz, proto_service_id s_id)
{
	memset(session.send_msg, 0, MAX_SMS_PL_SZ);
	session.send_msg[0] = SMSNAS_VERSION;
	session.send_msg[1] = s_id;
	session.send_msg[2] = (uint8_t)(total_sz & 0xff);
	session.send_msg[3] = (uint8_t)((total_sz >> 8) & 0xff);
	memcpy((session.send_msg + PROTO_OVERHEAD_SZ), payload, cp_sz);
}

/* Calculates number of segments for the given size when sz exceeds
 * SMS_SZ_WITHT_TUHD_WITH_PHD bytes of payload
 */

int calculate_total_msgs(proto_pl_sz sz)
{
	bool first = true;
	int total_msg = 1;
	int max_size = SMS_SZ_WITH_TUDH_WITH_PHD;
	while (sz > 0) {
		if (first) {
			sz = sz - max_size;
			total_msg++;
			max_size = SMS_SZ_WITH_TUHD_WITHT_PHD;
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

static void invoke_send_callback(const void *buf, proto_pl_sz sz,
				proto_service_id s_id, proto_callback cb,
				proto_result res)
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
 * if sz exceeds SMS_SZ_WITHT_TUHD_WITH_PHD bytes , it has to be sent as a
 * concatenated message
 */
proto_result smsnas_send_msg_to_cloud(const void *buf, proto_pl_sz sz,
					proto_service_id service_id,
					proto_callback cb)
{
	if (!buf || (sz == 0) || ((sz + PROTO_OVERHEAD_SZ) > PROTO_MAX_MSG_SZ))
		RETURN_ERROR("Invalid parameter", PROTO_INV_PARAM);

	/* flag gets set during smsnas_set_destination API call */
	if (!session.host_valid)
		RETURN_ERROR("Host is not valid", PROTO_INV_PARAM);

	uint8_t msg_ref_num = session.msg_ref_num;
	uint8_t total_msgs = 1;
	uint8_t cur_seq_num = 0;
	/* check if it needs to be concatenated message */
	if (sz <= SMS_SZ_WITHT_TUHD_WITH_PHD) {
		build_smsnas_msg(buf, sz, sz, service_id);
		proto_result res = write_to_modem(session.send_msg,
					sz + PROTO_OVERHEAD_SZ, msg_ref_num,
					total_msgs, cur_seq_num);
		if (res != PROTO_OK) {
			invoke_send_callback(buf, sz, service_id, cb, res);
			RETURN_ERROR("Send failed", PROTO_ERROR);
		}
		return PROTO_OK;
	}
	cur_seq_num = 1;
	total_msgs = calculate_total_msgs(sz + PROTO_OVERHEAD_SZ);
	if (total_msgs > 4)
		RETURN_ERROR("Send size exceeds", PROTO_ERROR);
	printf("%s:%d: Sending concatenated sms with total segs: %u\n",
						__func__, __LINE__, total_msgs);
	uint8_t *temp_buf = NULL;
	proto_pl_sz rem_sz = sz;
	proto_pl_sz send_sz = 0;
	proto_pl_sz total_sent = 0;
	while (1) {
		if (cur_seq_num == 1) {
			build_smsnas_msg(buf, sz, SMS_SZ_WITH_TUDH_WITH_PHD,
				service_id);
			temp_buf = session.send_msg;
			rem_sz = rem_sz - SMS_SZ_WITH_TUDH_WITH_PHD;
			send_sz = SMS_SZ_WITH_TUDH_WITH_PHD + PROTO_OVERHEAD_SZ;
		}
		proto_result ret = write_to_modem(temp_buf, send_sz,
					msg_ref_num, total_msgs, cur_seq_num);
		if (ret != PROTO_OK) {
			invoke_send_callback(buf, sz, service_id, cb, ret);
			RETURN_ERROR("Concatenated send failed", PROTO_ERROR);
		}
		/* Adjust user pointer buffer here for first segment to account
		 * for the protocol overhead size
		 */
		if (cur_seq_num == 1)
			total_sent += (send_sz - PROTO_OVERHEAD_SZ);
		else
			total_sent += send_sz;
		cur_seq_num++;
		if (cur_seq_num > total_msgs)
			break;
		temp_buf = (uint8_t *)buf + total_sent;
		if (rem_sz <= SMS_SZ_WITH_TUHD_WITHT_PHD)
			send_sz = rem_sz;
		else {
			rem_sz = rem_sz - SMS_SZ_WITH_TUHD_WITHT_PHD;
			send_sz = SMS_SZ_WITH_TUHD_WITHT_PHD;
		}
	}
	session.msg_ref_num = (msg_ref_num + 1) % MAX_SMS_REF_NUMBER;
	return PROTO_OK;
}

/* If conatenated message is in progress, returns timeout to poll for the
 * next segment
 */
uint64_t smsnas_get_polling_interval(void)
{
	return session.next_seg_rcv_timeout;
}
