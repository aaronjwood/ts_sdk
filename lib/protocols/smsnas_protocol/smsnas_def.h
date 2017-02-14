/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __SMSNAS_DEF
#define __SMSNAS_DEF

#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"

#define RECV_TIMEOUT_MS		5000
#define SEND_TIMEOUT_MS		5000
#define MULT			1000
#define SMSNAS_VERSION		0x1
#define SMSNAS_CTRL_MSG_VER	0x1
#define SMSNAS_MSG_TYPE_SLEEP	0x1

/* Max SMS size without user data header (TP-UDH) */
#define MAX_SMS_PL_SZ			140
#define MAX_SMS_PL_WITHOUT_HEADER	(MAX_SMS_PL_SZ - PROTO_OVERHEAD_SZ)

/* Defines for the concatenated sms */
/* 6 bytes for user data header element */
#define MAX_SMS_SZ_WITH_HEADER		(MAX_SMS_PL_SZ - PROTO_OVERHEAD_SZ - 6)
/* Account for TP userdata header and not the protocol header */
#define MAX_SMS_SZ_WITH_HD_WITHT_OVHD	(MAX_SMS_PL_SZ - 6)
#define MAX_SMS_REF_NUMBER		256
#define MAX_CONC_SMS_NUM		4

#define MAX_RETRIES			3

/* Defines payload */
typedef struct __attribute__((packed)) {
	proto_pl_sz sz;			/* Number of bytes currently filled */
	uint8_t bytes[];
} payload;

/* A complete SMSNAS protocol message */
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

typedef enum {
	IDLE = 0,

} conc_state;

typedef struct {
	uint8_t cref_num;
	uint8_t cur_seq;
	void *buf;
	proto_callback cb;
	proto_pl_sz total_sz;
	proto_pl_sz wr_idx;
	proto_pl_sz rem_sz;
	conc_state state;
} smsnas_rcv_path

static struct {
	bool host_valid;		/* Whether host contains valid string */
	char host[MAX_HOST_LEN + 1];	/* Store the host name */
	smsnas_rcv_path rcv_msg;
	uint8_t send_msg[MAX_SMS_PL_SZ];
} session;

#endif
