/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef __OTT_MSG
#define __OTT_MSG

/* Global header defines limits of the field sizes of OTT messages. Macro names
 * will be used globally and must not be changed
 */

#define PROTO_MAX_MSG_SZ		512
#define PROTO_VER_SZ		        1
#define PROTO_CMD_SZ		        1
#define PROTO_LEN_SZ                    2
#define PROTO_OVERHEAD_SZ		(PROTO_CMD_SZ + PROTO_LEN_SZ)
#define PROTO_DATA_SZ                   (PROTO_MAX_MSG_SZ - PROTO_OVERHEAD_SZ)

/* Private macro defines related to OTT protocol remote host and port maximum
 * length
 */
#define MAX_HOST_LEN	50	/**< Size of the host address string in bytes */
#define MAX_PORT_LEN	5	/**< Size of the port string in bytes */

#endif
