/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __SMSNAS_DEF
#define __SMSNAS_DEF

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"
#include "at_sms.h"

/* Macro indicates maximum concatenated sms receive stream protocol handles
 */

/*#define SMSNAS_MAX_RCV_PATH	2*/

#ifndef SMSNAS_MAX_RCV_PATH
#define SMSNAS_MAX_RCV_PATH	        1
#endif

/* Defines retries in case of send failure */
#define MAX_RETRIES			3

/* Version implemention of the smsnas protocol */
#define SMSNAS_VERSION		        0x1

/* Payload upper limit when TP-User header is not present and protocol header is
 * present
 */
#define SMS_SZ_WITHT_TUHD_WITH_PHD	(MAX_SMS_PL_SZ - PROTO_OVERHEAD_SZ)

/* Defines for the concatenated sms starts */

/* Payload upper limit when TP-User header is present and protocol header is not
 */
#define SMS_SZ_WITH_TUHD_WITHT_PHD	CONCT_SMS_SZ

/* Payload upper limit when TP-User header and protocol header are present
 */
#define SMS_SZ_WITH_TUDH_WITH_PHD	(SMS_SZ_WITH_TUHD_WITHT_PHD - \
                                                PROTO_OVERHEAD_SZ)

/* Upper limit for the total messages in the concatenated sms */
#define MAX_CONC_SMS_NUM		4

/* Time out waiting for the next segment of the concatenated sms in milliseconds
 */
#define CONC_NEXT_SEG_TIMEOUT_MS	2000
/* Defines for the concatenated sms ends */

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

/* Defines to enable printing of all the error strings */
#define DEBUG_ERROR
#ifdef DEBUG_ERROR
#define PRINTF_ERR(...)	printf(__VA_ARGS__)
#else
#define PRINTF_ERR(...)
#endif

/* Defines to enable printf to track function entry points */
/* #define DEBUG_FUNCTION */
#ifdef DEBUG_FUNCTION
#define PRINTF_FUNC(...)	printf(__VA_ARGS__)
#else
#define PRINTF_FUNC(...)
#endif

/* Defines flag for the ack/nack pending */
typedef enum {
	ACK_PENDING,
	NACK_PENDING,
	NO_ACK_NACK
} ack_nack;

/* SMSNAS protocol message, used to quickly retrieve protocol header values from
 * received data
 */
typedef struct __attribute__((packed)) {
	/* Protocol version bytes */
	uint8_t version;
	proto_service_id service_id;
	proto_pl_sz payload_sz;
	uint8_t payload[];
} smsnas_msg_t;

typedef struct {
	/* true if concatenated message is in progress */
	bool conct_in_progress;
	/* service id received from protocol message */
	proto_service_id service_id;
	/* message reference number in case of concatenated sms from
	 * tp-user header
	 */
	int cref_num;
	/* Current sequence number it is processing from concatenated sms */
	uint8_t cur_seq;
	/* Expected next sequence number within receive timeout */
	uint8_t expected_seq;
	/* Future Time interval within which next sequence from the concatenated
	 * sms should be received
	 */
	uint32_t next_seq_timeout;
	/* buffer to store receiving data */
	uint8_t *buf;
	/* write index of the buf */
	proto_pl_sz wr_idx;
	/* size of the receiving buffer */
	proto_pl_sz rcv_sz;
} smsnas_rcv_path;

static struct {
	/* message reference number of the outgoing concatenated sms */
	uint8_t msg_ref_num;
	/* Whether host contains valid string */
	bool host_valid;
	char host[MAX_HOST_LEN + 1];
	/* Number of receive paths for simultaneously receving multiple
	 * concatenated sms's
	 */
	smsnas_rcv_path rcv_msg[SMSNAS_MAX_RCV_PATH];
	/* outgoing buffer to hold user data plus smsnas protocol header data */
	uint8_t send_msg[MAX_SMS_PL_SZ];
	/* True when user has called to schedule any future incoming messages */
	bool rcv_valid;
	/* Used to hold user supplied incoming data buffer especially when
	 * SMSNAS_MAX_RCV_PATH > 1 for which case this layer uses its internal
	 * buffer to hold data before transferring to rcv_buf
	 */
	void *rcv_buf;
	/* Size of the user supplied incoming buffer */
	proto_pl_sz rcv_sz;
	/* callback of the user supplied incoming buffer, when data is received */
	proto_callback rcv_cb;
	/* Keeps track of the polling interval for the next segment of the
	 * concatenated sms, this variable gets passed to user to call in for
	 * future time to check back if protocol layer has received next segment
	 * if not, then call receive time out
	 */
	uint32_t next_seg_rcv_timeout;
	/* variable to keep track of the pending ack/nack */
	ack_nack ack_nack_pend;
} session;

#endif
