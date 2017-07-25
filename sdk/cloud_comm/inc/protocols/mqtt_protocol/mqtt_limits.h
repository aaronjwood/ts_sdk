/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __MQTT_LIM
#define __MQTT_LIM

/* Header defines limits of the field sizes of MQTT messages. Macro names
 * will be used globally and must not be changed, right now there is no
 * verizon defined meta data for the mqtt which leaves burden of decoding
 * to the user application
 */


/* FIXME: This value needs to be discussed */
#define PROTO_MAX_MSG_SZ		2024

#define PROTO_OVERHEAD_SZ		0
#define PROTO_DATA_SZ                   (PROTO_MAX_MSG_SZ - PROTO_OVERHEAD_SZ)

#define PROTO_MAX_SEND_BUF_SZ           PROTO_DATA_SZ
#define PROTO_MAX_RECV_BUF_SZ           PROTO_DATA_SZ

#endif
