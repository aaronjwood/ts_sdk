/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __SMSNAS_LIM
#define __SMSNAS_LIM

/* Global header defines limits of the field sizes of OTT messages. Macro names
 * will be used globally and must not be changed
 */

#define PROTO_MAX_MSG_SZ	512

#define PROTO_VER_SZ		1
#define PROTO_SERV_ID_SZ        1
#define PROTO_LEN_SZ            2
#define PROTO_OVERHEAD_SZ	(PROTO_VER_SZ + PROTO_SERV_ID_SZ + PROTO_LEN_SZ)
#define PROTO_DATA_SZ           (PROTO_MAX_MSG_SZ - PROTO_OVERHEAD_SZ)

#define PROTO_MIN_RECV_BUF_SZ           5 /* command byte + sizeof(uint32_t) */
#define PROTO_MIN_SEND_BUF_SZ           1 /* command byte */
#define PROTO_MAX_SEND_BUF_SZ           PROTO_DATA_SZ
#define PROTO_MAX_RECV_BUF_SZ           PROTO_DATA_SZ

/* Macro indicates maximum receive stream protocol handles */
#define PROTO_MAX_RCV_PATH              2
#endif
