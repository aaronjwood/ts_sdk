/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#ifndef __OTT_LIM
#define __OTT_LIM

/* Global header defines limits of the field sizes of OTT messages. Macro names
 * will be used globally and must not be changed
 */

#define PROTO_MAX_MSG_SZ		512
#define PROTO_VER_SZ		        1
#define PROTO_CMD_SZ		        1
#define PROTO_LEN_SZ                    2
#define PROTO_OVERHEAD_SZ		(PROTO_CMD_SZ + PROTO_LEN_SZ)
#define PROTO_DATA_SZ                   (PROTO_MAX_MSG_SZ - PROTO_OVERHEAD_SZ)

#define PROTO_MIN_RECV_BUF_SZ           5
#define PROTO_MIN_SEND_BUF_SZ           1
#define PROTO_MAX_SEND_BUF_SZ           PROTO_DATA_SZ
#define PROTO_MAX_RECV_BUF_SZ           PROTO_DATA_SZ

#endif
