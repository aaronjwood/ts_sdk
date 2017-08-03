/* Copyright (C) 2017 Verizon. All rights reserved. */

#ifndef AT_SQMONARCH_TCP_COMMAND_H
#define AT_SQMONARCH_TCP_COMMAND_H

#include "at_core.h"
#include "ts_sdk_modem_config.h"

enum at_urc {
	TCP_CLOSED,	/* TCP connection closed URC */
	URC_END
};

enum tcp_command {
	PDP_ACT,	/* Activate PDP context */
	SOCK_CONF,	/* Configure socket */
	SOCK_CONF_EXT,	/* Configure extended socket parameters */
	SOCK_DIAL,	/* Dial into a hostname:port / IP address:port */
	SOCK_CLOSE,	/* Close the socket */
	SOCK_SUSP,	/* Suspend online mode */
	SOCK_RESM,	/* Resume online mode */
	TCP_END
};

static const char *at_urcs[URC_END] = {
	[TCP_CLOSED] = "\r\n+SQNSH: "MODEM_SOCK_ID"\r\n"
};

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
		.comm = "at+sqnscfg="MODEM_SOCK_ID","MODEM_PDP_CTX",0,600,600,50\r",
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
		.comm = "at+sqnscfgext="MODEM_SOCK_ID",0,0,0,0,0\r",
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
		.comm_sketch = "at+sqnsd="MODEM_SOCK_ID",0,%s,\"%s\",0,0,0\r",
		.rsp_desc = {
			{
				.rsp = "\r\nCONNECT\r\n",
				.rsp_handler = NULL,
				.data= NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 7000
	},
	[SOCK_SUSP] = {
		.comm = "+++",
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
	[SOCK_RESM] = {
		.comm = "at+sqnso="MODEM_SOCK_ID"\r",
		.rsp_desc = {
			{
				.rsp = "\r\nCONNECT\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 5000
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
	}
};

#endif
