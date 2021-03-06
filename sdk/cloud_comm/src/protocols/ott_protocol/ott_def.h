/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __OTT_DEF
#define __OTT_DEF

#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"

#ifdef MODEM_SQMONARCH
#define RECV_TIMEOUT_MS		30000
#define TIMEOUT_MS		30000
#else
#define RECV_TIMEOUT_MS		5000
#define TIMEOUT_MS		5000
#endif

#define MULT			1000
#define INIT_POLLING_MS		((uint32_t)15000)
#define VERSION_BYTE		((uint8_t)0x01)

#define OTT_UUID_SZ             16 /* unique device id size in bytes */
#define OTT_DEV_SC_SZ		32 /* device secret size in bytes */

#define MAX_HOST_LEN		50
#define MAX_PORT_LEN		5

#define UPD_OVR_HEAD		PROTO_OVERHEAD_SZ
#define MIN_UPD_SIZE		(UPD_OVR_HEAD + 1 /* Actual data */)
#define MIN_CMD_PI_SIZE		(PROTO_CMD_SZ + 4 /* uint32_t */)
#define MIN_CMD_SL_SIZE		(PROTO_CMD_SZ + 4 /* uint32_t */)
#define MIN_MT_NONE_SIZE	PROTO_CMD_SZ

typedef enum {			/* Defines control flags. */
	CF_NONE = 0x00,		/* No flag set */
	CF_NACK = 0x10,		/* Failed to accept or process previous message */
	CF_ACK = 0x20,		/* Previous message accepted */
	CF_PENDING = 0x40,	/* More messages to follow */
	CF_QUIT = 0x80		/* Close connection */
} c_flags_t;

typedef enum  {			/* Defines message type flags. */
	MT_NONE = 0,		/* Control message */
	MT_AUTH = 1,		/* Authentication message sent by device */
	MT_STATUS = 2,		/* Status report sent by device */
	MT_UPDATE = 3,		/* Update message received by device */
	MT_RESTARTED = 4,	/* Lets the cloud know the device restarted */
	MT_CMD_PI = 10,		/* Cloud instructs to set new polling interval */
	MT_CMD_SL = 11		/* Cloud instructs device to sleep */
} m_type_t;

/* Helper macros to query if certain flag bits are set in the command byte. */
#define OTT_FLAG_IS_SET(var, flag)	\
	(((flag) == CF_NONE) ? ((var) == CF_NONE) : ((var) & (flag)) == (flag))

/* Helper macros to interpret the command byte. */
#define OTT_LOAD_FLAGS(cmd, f_var)	((f_var) = (uint8_t)(cmd) & 0xF0)
#define OTT_LOAD_MTYPE(cmd, m_var)	((m_var) = (uint8_t)(cmd) & 0x0F)

/* Defines an array type */
typedef struct __attribute__((packed)) {
	uint16_t sz;			/* Number of bytes currently filled */
	uint8_t bytes[];
} array_t;

/* Defines a value received by the device from the cloud */
typedef union __attribute__((packed)) {
	uint32_t interval;
	array_t array;
} msg_packet_t;

/* A complete OTT protocol message */
typedef struct __attribute__((packed)) {
	uint8_t cmd_byte;
	msg_packet_t data;
} msg_t;

static struct {				/* Store authentication data */
	bool auth_valid;		/* true if dev_id and dev_sec contains
					 * valid information
					 */
	bool serv_auth_valid;		/* Server side verification credentials
					 * valid
					 */
	uint8_t dev_ID[OTT_UUID_SZ];	/* 16 byte Device ID */
	uint8_t d_sec[OTT_DEV_SC_SZ];	/* Device secret */
} auth;

static struct {
	bool conn_done;			/* TCP connection was established */
	bool auth_done;			/* Auth was sent for this session */
	bool pend_bit;			/* Cloud has a pending message */
	bool pend_ack;			/* Set if the device needs to ACK prev msg */
	bool nack_sent;			/* Set if a NACK was sent from the device */
	char host[MAX_HOST_LEN + 1];	/* Store the host name */
	char port[MAX_PORT_LEN + 1];	/* Store the host port */
	proto_service_id send_svc_id;	/* Service id of last sent message */
	msg_t *rcv_buf;
	uint32_t rcv_sz;
	const void *send_buf;
	uint32_t send_sz;
	proto_callback rcv_cb;
	proto_callback send_cb;
} session;

/*
 * Define this to profile the maximum heap used by mbedTLS. This is done by
 * providing a custom implementation of calloc(). Everytime the connection is
 * closed through ott_close_connection(), the maximum amount of heap used until
 * that point of time is printed to the debug UART.
 */
/*#define OTT_HEAP_PROFILE*/

#endif
