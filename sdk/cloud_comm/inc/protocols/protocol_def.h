/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __PROTOCOL_DEF
#define __PROTOCOL_DEF

#include <stdint.h>

#if defined (OTT_PROTOCOL)
#include "ott_limits.h"
#elif defined (SMSNAS_PROTOCOL)
#include "smsnas_limits.h"
#elif defined (MQTT_PROTOCOL)
#include "mqtt_limits.h"
#else
#error "define valid protocol options from OTT_PROTOCOL or SMSNAS_PROTOCOL"
#endif

/* Superset of the protocol API execution results, depending on the protocols
 * implemented or introduced, this list can be modified to accomodate various
 * API related execution result codes
 */
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
	PROTO_RCVD_NACK,	/* Received a NACK for the last message sent */
	PROTO_SEND_TIMEOUT,	/* Timed out waiting for a response */
	PROTO_SEND_FAILED,	/* Sending message failed */

	/* Incoming message events: */
	/* Insufficient memory to store received message */
	PROTO_RCVD_MEM_OVRFL,
	/* Received quit (i.e. session termination) */
	PROTO_RCVD_QUIT,
	/* Received message out of protocol spec or defines */
	PROTO_RCVD_WRONG_MSG,
	/* Received a message from the cloud */
	PROTO_RCVD_MSG
} proto_event;

typedef uint16_t proto_pl_sz; /* type representing total payload size */

typedef uint8_t proto_service_id;

/*
 * Pointer to a callback routine. The callback accepts a buffer, its size,
 * an event from the source of the callback explaining why it was invoked,
 * and the service id of the service that should process the event.
 * Note: Some protocols do not support service id concept in which case it should
 * be ingnored.
 */
typedef void (*proto_callback)(const void *buf, uint32_t sz,
			       proto_event event, proto_service_id svc_id);

/*
 * Define this to profile function execution time
 *
 */
/*#define PROTO_TIME_PROFILE*/

/*
 * Define this to explicitly show the time it takes to connect to the network /
 * send / receive data over the network.
 * PROTO_TIME_PROFILE must be defined for this to work.
 */
/*#define PROTO_EXPLICIT_NETWORK_TIME*/


#ifdef PROTO_TIME_PROFILE
#ifdef PROTO_EXPLICIT_NETWORK_TIME

extern uint32_t network_time_ms;

#define PROTO_TIME_PROFILE_BEGIN() do { \
	proto_begin = sys_get_tick_ms(); \
	network_time_ms = 0; \
} while(0)

#define PROTO_TIME_PROFILE_END(label) do { \
	dbg_printf("["label":%u]", sys_get_tick_ms() - proto_begin); \
	dbg_printf(" [NETW:%u]\n", network_time_ms); \
} while(0)

#else

#define PROTO_TIME_PROFILE_BEGIN() \
	proto_begin = sys_get_tick_ms()

#define PROTO_TIME_PROFILE_END(label) \
	dbg_printf("["label":%u]\n", sys_get_tick_ms() - proto_begin)

#endif	/* PROTO_EXPLICIT_NETWORK_TIME */

#else

#define PROTO_TIME_PROFILE_BEGIN()
#define PROTO_TIME_PROFILE_END(label)

#endif	/* PROTO_TIME_PROFILE */

#endif
