/* Copyright (C) 2017 Verizon. All rights reserved. */

#ifndef AT_SQMONARCH_TCP_COMMAND_H
#define AT_SQMONARCH_TCP_COMMAND_H

#include "at_core.h"
#include "ts_sdk_modem_config.h"

enum at_urc {
	TCP_CLOSED,	/* TCP connection closed URC */
	TCP_RECV,	/* Data received by modem URC */
	URC_END
};

enum tcp_command {
	PDP_ACT,	/* Activate PDP context */
	SOCK_CONF,	/* Configure socket */
	SOCK_CONF_EXT,	/* Configure extended socket parameters */
	SOCK_DIAL,	/* Dial into a hostname:port / IP address:port */
	SOCK_SEND,	/* Send data (up to 1500 bytes) over a TCP socket */
	SOCK_SEND_DATA,	/* Actual data to send */
	SOCK_CLOSE,	/* Close the socket */
	GET_IP_ADDR,	/* Get the IP address */
	TCP_END
};

static const char *at_urcs[URC_END] = {
	[TCP_CLOSED] = "\r\n+SQNSH: "MODEM_SOCK_ID"\r\n",
	[TCP_RECV] = "\r\n+SQNSRING: "MODEM_SOCK_ID","	/* Followed by "length,data" */
};

static void parse_ip_addr(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data);

/* XXX: Data sheet does not specify timeouts. They're arbitrary for now. */
static at_command_desc tcp_commands[TCP_END] = {
	[PDP_ACT] = {
		.comm = "at+cgact=1,"MODEM_PDP_CTX"\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 5000
	},
	[SOCK_CONF] = {
		.comm = "at+sqnscfg="MODEM_SOCK_ID","MODEM_PDP_CTX",1500,600,600,50\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 5000
	},
	[SOCK_CONF_EXT] = {
		.comm = "at+sqnscfgext="MODEM_SOCK_ID",2,0,0,0,0\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 5000
	},
	[SOCK_DIAL] = {
		.comm_sketch = "at+sqnsd="MODEM_SOCK_ID",0,%s,\"%s\",0,0,1\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 7000
	},
	[SOCK_SEND] = {
		.comm = "at+sqnssend="MODEM_SOCK_ID"\r",
		.rsp_desc = {
			{
				.rsp = "\r\n> ",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 5000
	},
	[SOCK_SEND_DATA] = {
		.comm = (char []){0x1A, 0x00},	/* Ctrl+Z (0x1A) to initiate send */
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 10000
	},
	[SOCK_CLOSE] = {
		.comm = "at+sqnsh="MODEM_SOCK_ID"\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 5000
	},
	[GET_IP_ADDR] = {
		.comm = "at+cgpaddr="MODEM_PDP_CTX"\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+CGPADDR: "MODEM_PDP_CTX",\"",
				.rsp_handler = parse_ip_addr,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 5000
	}
};

#endif
