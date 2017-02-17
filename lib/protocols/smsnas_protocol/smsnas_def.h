/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __SMSNAS_DEF
#define __SMSNAS_DEF

#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"

#define MAX_HOST_LEN		ADDR_SZ
#define INIT_POLLING_MS		0

#define RECV_TIMEOUT_MS		5000
#define SEND_TIMEOUT_MS		5000
#define MULT			1000
#define SMSNAS_VERSION		0x1
#define SMSNAS_CTRL_MSG_VER	0x1
#define SMSNAS_MSG_TYPE_SLEEP	0x1

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

/* Time out waiting for the next segment for the concatenated message */
#define CONC_NEXT_SEG_TIMEOUT_MS	2000

/* Defines retries in case of send failure */
#define MAX_RETRIES			3

/* Defines flag for the ack/nack pending */
#define ACK_PENDING			1
#define NACK_PENDING			2

/* Defines payload */
typedef struct __attribute__((packed)) {
	proto_pl_sz sz;			/* Number of bytes currently filled */
	uint8_t bytes[];
} payload;

/* SMSNAS protocol message */
typedef struct __attribute__((packed)) {
	uint8_t version;
	uint8_t service_id;
	payload data;
} smsnas_msg_t;

/* Defines a value received by the device from the cloud */
typedef union __attribute__((packed)) {
	uint32_t interval;
	payload data;
} ctrl_msg_t;

typedef struct __attribute__((packed)) {
	uint8_t version;
	uint8_t msg_type;
	ctrl_msg_t msg;
} smsnas_ctrl_msg;

typedef struct {
	bool rcv_path_valid;
	bool conct_in_progress;
	uint8_t service_id;
	uint8_t ack_nack_pend;
	uint8_t cref_num;
	uint8_t cur_seq;
	uint8_t expected_seq;
	void *buf;
	proto_callback cb;
	proto_pl_sz wr_idx;
	proto_pl_sz rem_sz;
} smsnas_rcv_path

static struct {
	bool host_valid;		/* Whether host contains valid string */
	char host[MAX_HOST_LEN + 1];	/* Store the host name */
	smsnas_rcv_path rcv_msg;
	uint8_t send_msg[MAX_SMS_PL_SZ];
} session;

#endif
