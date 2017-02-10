/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __SMSNAS_DEF
#define __SMSNAS_DEF

#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"

#define RECV_TIMEOUT_MS		5000
#define SEND_TIMEOUT_MS		5000
#define MULT			1000
#define VERSION_BYTE		((uint8_t)0x01)

/* Max SMS size without user data header (TP-UDH) */
#define MAX_SMS_PL_SZ			140
#define MAX_SMS_PL_WITHOUT_HEADER	(MAX_SMS_PL_SZ - PROTO_OVERHEAD_SZ)

/* Defines for the concatenated sms */
/* 6 bytes for user data header element */
#define MAX_SMS_SZ_WITH_HEADER		(MAX_SMS_PL_SZ - PROTO_OVERHEAD_SZ - 6)
#define MAX_SMS_REF_NUMBER		256
#define MAX_CONC_SMS_NUM		4

#define MAX_RETRIES		3

/* A complete SMSNAS protocol message */
typedef struct __attribute__((packed)) {
	uint8_t version;
	uint8_t service_id;
	uint16_t payload_sz;
	uint8_t payload[];
} msg_t;

static struct {
	bool host_valid;		/* Whether host contains valid string */
	char host[MAX_HOST_LEN + 1];	/* Store the host name */
	msg_t *rcv_buf;			/* pointer to store the received bytes */
	uint8_t send_msg[MAX_SMS_PL_SZ];
	proto_pl_sz rcv_sz;
	proto_callback rcv_cb;
} session;

#endif
