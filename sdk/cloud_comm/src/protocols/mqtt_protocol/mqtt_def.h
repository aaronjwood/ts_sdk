/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __MQTT_DEF
#define __MQTT_DEF

#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"

#define MAX_HOST_LEN		50
#define MAX_PORT_LEN		5

/* optional interface parameters, for linux devices this is required for others
 * this can be ignored
 */
#define NET_INTERFACE		"eth0"

/* This overhead is fixed + variable header size based on selected mqtt protocol
 * version 3 and it is dictated by low level mqtt protocol
 */
#define MQTT_OVERHEAD_SZ	14

#define MQTT_SEND_SZ		(MQTT_OVERHEAD_SZ + PROTO_MAX_MSG_SZ)
#define MQTT_RCV_SZ		(MQTT_OVERHEAD_SZ + PROTO_MAX_MSG_SZ)

/* defines related to mqtt protocol */
#define MQTT_WILL		0
#define MQTT_CLEAN_SESSION	1
#define MQTT_PROTO_VERSION	3
#define MQTT_KEEPALIVE_INT_SEC	60
#define MQTT_QOS_LVL		QOS1
#define MQTT_SERV_PUBL_COMMAND         	"ThingspaceSDK/%s/TSServerPublishCommand"
#define MQTT_PUBL_UNIT_ON_BOARD   	"ThingspaceSDK/%s/UNITOnBoard"
#define MQTT_PUBL_CMD_RESPONSE    	"ThingspaceSDK/%s/UNITCmdResponse"

#define MQTT_TOPIC_SZ 			100
#define MQTT_DEVICE_ID_SZ		14

#define SEC_TO_MS		1000

#define INIT_POLLING_MS		((uint32_t)(MQTT_KEEPALIVE_INT_SEC * SEC_TO_MS))


/* Special care needs to be taken care for cat m1 modems as they are slow */
#ifdef MODEM_SQMONARCH
#define MQTT_TIMEOUT_MS		30000
#else
#define MQTT_TIMEOUT_MS		1000
#endif


/* Defines to enable printing of all the error strings */
#define DEBUG_ERROR
#ifdef DEBUG_ERROR
#define PRINTF_ERR(...)	printf(__VA_ARGS__)
#else
#define PRINTF_ERR(...)
#endif

/* Enables displaying major milestones achieved in flow */
#define DEBUG_MSG
#ifdef DEBUG_MSG
#define PRINTF(...)	printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* Defines to enable printf to track function entry points */
#define DEBUG_FUNCTION
#ifdef DEBUG_FUNCTION
#define PRINTF_FUNC(...)	printf(__VA_ARGS__)
#else
#define PRINTF_FUNC(...)
#endif

static struct {
	bool conn_valid;		/* TCP connection was established */
	bool own_auth_valid;		/* Client credentials are set and valid */
	bool remote_auth_valid;		/* Remote credentials are set and valid */
	bool host_valid;		/* host:port arrays content is valid */
	char host[MAX_HOST_LEN + 1];	/* Store the host name */
	char port[MAX_PORT_LEN + 1];	/* Store the host port */
	proto_service_id send_svc_id;	/* Service id of last sent message */
	uint8_t *rcv_buf;		/* receive buffer */
	uint32_t rcv_sz;		/* size in bytes for above buffer */
	uint64_t msg_sent;		/* timestamp when last message was sent */
	proto_callback rcv_cb;		/* receive callback */
	proto_callback send_cb;		/* send callback */
} session;

/*
 * Define this to profile the maximum heap used by mbedTLS. This is done by
 * providing a custom implementation of calloc(). Everytime the connection is
 * closed through ott_close_connection(), the maximum amount of heap used until
 * that point of time is printed to the debug UART.
 */
/*#define MQTT_HEAP_PROFILE*/

#endif
