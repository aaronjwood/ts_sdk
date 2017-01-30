/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef __PROTOCOL
#define __PROTOCOL

#include <stdint.h>
#include <stdbool.h>
#include "dbg.h"

/* Superset of the protocol API execution results */
typedef enum {			/* Defines return codes of this API. */
	PROTO_OK,		/* API call exited without any errors */
	PROTO_ERROR,		/* API call exited with errors */
	PROTO_TIMEOUT,		/* Timed out waiting for API to finish */
	PROTO_NO_MSG,		/* There are no messages to be retrieved */
	PROTO_INV_PARAM		/* Invalid parameters passed to the API */
} proto_result;

/*
 * Events delivered to the send and receive callback routines. This will be a
 * superset of all the types of events supported by underlying protocols
 */
typedef enum {
	PROTO_RCVD_NONE,		/* Default value; No message body */
	/* Outgoing message events: */
	PROTO_RCVD_ACK,		/* Received an ACK for the last message sent */
	PROTO_RCVD_NACK,		/* Received a NACK for the last message sent */
	PROTO_SEND_TIMEOUT,	/* Timed out waiting for a response */

	/* Incoming message events: */
	PROTO_RCVD_CMD_SL,	/* Received sleep time from the cloud */
	PROTO_RCVD_CMD_PI,	/* Received polling interval from the cloud */
	PROTO_RCVD_QUIT,	/* Received quit */
	PROTO_RCVD_RCV_UPD	/* Received an update message from the cloud */
} proto_event;

typedef uint32_t proto_buf_sz;
typedef void (*proto_callback)(const void *buf, proto_buf_sz sz,
                                                proto_event event);

#ifdef OTT_PROTOCOL
#include "ott_limits.h"
#include "ott_protocol.h"
#elseif SMSNAS_PROTOCOL
#include "smsnas_limits.h"
#include "smsnas_protocol.h"
#else
Please define protocol to use, valid options OTT_PROTOCOL or SMSNAS_PROTOCOL
#endif

/*
 * Define this to profile the OTT functions. The time recorded also includes the
 * time taken to receive the data over the network and time spent in the modem.
 * The initialization function is not profiled.
 */
/*#define PROTO_TIME_PROFILE*/

/*
 * Define this to explicitly show the time it takes to connect to the network /
 * send / receive data over the network. This allows to compute how much time is
 * spent in pure crypto computation / preparing and unwrapping the data packets.
 * OTT_TIME_PROFILE must be defined for this to work.
 */
/*#define PROTO_EXPLICIT_NETWORK_TIME*/

/*
 * Define this to profile the maximum heap used by mbedTLS. This is done by
 * providing a custom implementation of calloc(). Everytime the connection is
 * closed through ott_close_connection(), the maximum amount of heap used until
 * that point of time is printed to the debug UART.
 */
/*#define PROTO_HEAP_PROFILE*/

#ifdef PROTO_TIME_PROFILE
#ifdef PROTO_EXPLICIT_NETWORK_TIME

extern uint32_t network_time_ms;

#define PROTO_TIME_PROFILE_BEGIN() do { \
	proto_begin = platform_get_tick_ms(); \
	network_time_ms = 0; \
} while(0)

#define PROTO_TIME_PROFILE_END(label) do { \
	dbg_printf("["label":%u]", platform_get_tick_ms() - proto_begin); \
	dbg_printf(" [NETW:%u]\n", network_time_ms); \
} while(0)

#else

#define PROTO_TIME_PROFILE_BEGIN() \
	proto_begin = platform_get_tick_ms()

#define PROTO_TIME_PROFILE_END(label) \
	dbg_printf("["label":%u]\n", platform_get_tick_ms() - proto_begin)

#endif	/* PROTO_EXPLICIT_NETWORK_TIME */

#else

#define PROTO_TIME_PROFILE_BEGIN()
#define PROTO_TIME_PROFILE_END(label)

#endif	/* PROTO_TIME_PROFILE */

static inline proto_result proto_init()
{
#ifdef OTT_PROTOCOL
                return ott_protocol_init();
#elseif SMSNAS_PROTOCOL
                return PROTO_OK;
#endif
}

static inline cost uint8_t *proto_get_rcvd_msg_ptr(void *msg)
{
#ifdef OTT_PROTOCOL
        return ott_get_rcv_buffer(msg);
#elseif SMSNAS_PROTOCOL
        return NULL;
#endif
}

static inline proto_result proto_set_auth(const uint8_t *d_ID,
                                        uint16_t d_sec_sz, const uint8_t *d_sec)
{
#ifdef OTT_PROTOCOL
        return ott_set_auth(d_ID, d_sec_sz, d_sec);
#else
        return PROTO_OK;
#endif
}

static inline uint32_t proto_get_default_polling()
{
#ifdef OTT_PROTOCOL
        return ott_get_default_polling();
#else
        return 0;
#endif
}

static inline void proto_initiate_quit(bool send_nack)
{
#ifdef OTT_PROTOCOL
        ott_initiate_quit(send_nack);
#endif
}

static inline uint32_t proto_get_sleep_interval(void *msg)
{
#ifdef OTT_PROTOCOL
        return ott_get_sleep_interval(msg);
#elseif SMSNAS_PROTOCOL
        return 0;
#endif
}

static inline uint32_t proto_get_rcvd_data_len(void *msg)
{
#ifdef OTT_PROTOCOL
        return ott_get_rcvd_data_len(msg);
#elseif SMSNAS_PROTOCOL
        return 0;
#endif
}

static inline proto_result proto_resend_init_config(proto_callback cb)
{
#ifdef OTT_PROTOCOL
        return ott_resend_init_config(cb);
#elseif SMSNAS_PROTOCOL
        return PROTO_OK;
#endif
}

static inline proto_result proto_send_msg_to_cloud(void *msg, uint32_t sz,
                                        proto_callback cb)
{
#ifdef OTT_PROTOCOL
        return ott_send_bytes_to_cloud(msg, sz, cb);
#elseif SMSNAS_PROTOCOL
        return PROTO_OK;
#endif
}

static inline void proto_send_ack()
{
#ifdef OTT_PROTOCOL
        ott_send_ack();
#elseif SMSNAS_PROTOCOL
        return;
#endif
}

static inline void proto_send_nack()
{
#ifdef OTT_PROTOCOL
        ott_send_nack();
#elseif SMSNAS_PROTOCOL
        return;
#endif
}

static inline proto_status proto_set_recv_buffer_cb(void *rcv_buf, uint32_t sz,
                                                        proto_callback rcv_cb)
{

#ifdef OTT_PROTOCOL
        return ott_set_recv_buffer(rcv_buf, sz, rcv_cb);
#elseif SMSNAS_PROTOCOL
        return PROTO_OK;
#endif
}

static void proto_maintenance(bool poll_due)
{
#ifdef OTT_PROTOCOL
        ott_maintenance(poll_due);
#elseif SMSNAS_PROTOCOL
        return;
#endif
}

#endif
