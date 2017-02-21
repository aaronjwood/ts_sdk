/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __SMSNAS_DEF
#define __SMSNAS_DEF

#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"

/* Macro indicates maximum receive stream protocol handles, if more than 1
 * define intermediate receive buffer with maximum size times receive stream
 * if it is not defined or less then 2, use buffer provided from upper level to
 * store receving data
 */
#define SMSNAS_MAX_RCV_PATH	2

#ifndef SMSNAS_MAX_RCV_PATH
#define SMSNAS_MAX_RCV_PATH	1
#endif

/* If more then one, define intermediate buffer to store incoming messages */
#if SMSNAS_MAX_RCV_PATH > 1
uint8_t smsnas_rcv_buf[PROTO_MAX_MSG_SZ * SMSNAS_MAX_RCV_PATH];
#endif

#define MAX_HOST_LEN		ADDR_SZ
#define INIT_POLLING_MS		0

#define RECV_TIMEOUT_MS		5000
#define SEND_TIMEOUT_MS		5000
#define MULT			1000
#define SMSNAS_VERSION		0x1

/* Max SMS size without user data header (TP-UDH) */
#define MAX_SMS_PL_SZ			140

/* Payload upper limit when TP-User header is not present */
#define MAX_SMS_PL_WITHOUT_HEADER	(MAX_SMS_PL_SZ - PROTO_OVERHEAD_SZ)

/* Defines for the concatenated sms */
/* Size of the user data header element */
#define TP_USER_HEADER_SZ		6
#define MAX_SMS_SZ_WITH_HEADER		(MAX_SMS_PL_SZ - PROTO_OVERHEAD_SZ - TP_USER_HEADER_SZ)

/* Account for TP userdata header and not the protocol header overhead */
#define MAX_SMS_SZ_WITH_HD_WITHT_OVHD	(MAX_SMS_PL_SZ - TP_USER_HEADER_SZ)

/* 8bit reference number for the concatenated sms */
#define MAX_SMS_REF_NUMBER		256

/* Upper limit for the total messages in the concatenated sms */
#define MAX_CONC_SMS_NUM		4

/* Time out waiting for the next segment of the concatenated sms in milliseconds
 */
#define CONC_NEXT_SEG_TIMEOUT_MS	2000

/* Defines retries in case of send failure */
#define MAX_RETRIES			3

/* Defines flag for the ack/nack pending */
typedef enum {
	ACK_PENDING,
	NACK_PENDING,
	NO_ACK_NACK
} ack_nack;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

/* SMSNAS protocol message */
typedef struct __attribute__((packed)) {
	uint8_t version;
	uint8_t service_id;
	proto_pl_sz payload_sz;
	uint8_t payload[];
} smsnas_msg_t;

typedef struct {
	bool rcv_path_valid;
	bool conct_in_progress;
	uint8_t service_id;
	int cref_num;
	uint8_t cur_seq;
	uint8_t expected_seq;
	uint8_t *buf;
	uint32_t init_timestamp;
	uint32_t next_seq_timeout;
	proto_pl_sz wr_idx;
	proto_pl_sz rem_sz;
} smsnas_rcv_path;

static struct {
	uint8_t msg_ref_num;
	bool host_valid;		/* Whether host contains valid string */
	char host[MAX_HOST_LEN + 1];	/* Store the host name */
	smsnas_rcv_path rcv_msg[SMSNAS_MAX_RCV_PATH];
	uint8_t send_msg[MAX_SMS_PL_SZ];
	bool rcv_valid;
	void *rcv_buf;
	proto_pl_sz rcv_sz;
	proto_callback rcv_cb;
	uint32_t cur_polling_interval;
	ack_nack ack_nack_pend;
} session;

#endif
