/* Copyright (C) 2017 Verizon. All rights reserved. */

#ifndef AT_TOBY_SMS_COMMAND_H
#define AT_TOBY_SMS_COMMAND_H

#include "at_core.h"
#include "ts_sdk_modem_config.h"

#define NET_STAT_REG_CODE	6
enum at_modem_network_commands {
	NET_REG_QUERY,
	NET_REG_URC_SET,
	MNO_CONF_QUERY,
	MNO_CONF_SET,
	NUM_MODEM_COMMANDS
};

enum at_modem_sms_commands {
	SMS_ENTER_PDU_MODE,
	SMS_SEND,
	SMS_SEND_DATA,
	SMS_SET_CSMS,
	SMS_SET_CNMI,
	SMS_SEND_ACK,
	SMS_SEND_NACK,
	SMS_DEL_ALL_MSG,
	NUM_SMS_COMMANDS
};

typedef enum at_urc {
	NET_STAT_URC,
	CMT_URC,
	NUM_URCS
} at_urc;

static const char *at_urcs[NUM_URCS] = {
	[NET_STAT_URC] = "\r\n+CREG: ",
	[CMT_URC] = "\r\n+CMT: ,"
};

static at_command_desc sms_cmd[NUM_SMS_COMMANDS] = {
	[SMS_ENTER_PDU_MODE] = {
		.comm = "at+cmgf=0\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = NULL,
		.comm_timeout = 100
	},
	[SMS_SEND] = {
		.comm_sketch = "at+cmgs=%u\r",
		.rsp_desc = {
			{
				.rsp = "\r\n> ",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CMS ERROR: ",
		.comm_timeout = 1000
	},
	[SMS_SEND_DATA] = {
		.comm = NULL,	/* Filled later with PDU string in at_sms_send */
		.rsp_desc = {
			{
				.rsp = "\r\n+CMGS: ",
				.rsp_handler = NULL,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CMS ERROR: ",
		.comm_timeout = 150000
	},
	[SMS_SET_CSMS] = {
		.comm = "at+csms=1\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+CSMS: 1,1,1\r\n",
				.rsp_handler = NULL,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CMS ERROR: ",
		.comm_timeout = 100
	},
	[SMS_SET_CNMI] = {
		.comm = "at+cnmi=1,2,0,0,0\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CMS ERROR: ",
		.comm_timeout = 100
	},
	[SMS_SEND_ACK] = {
		.comm = "at+cnma=1\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CMS ERROR: ",
		.comm_timeout = 150000
	},
	[SMS_SEND_NACK] = {
		.comm = "at+cnma=2\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CMS ERROR: ",
		.comm_timeout = 150000
	},
	[SMS_DEL_ALL_MSG] = {
		.comm = "at+cmgd=1,4\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CMS ERROR: ",
		.comm_timeout = 55000
	}
};

static const at_command_desc mod_netw_cmd[NUM_MODEM_COMMANDS] = {
	[NET_REG_QUERY] = {
		.comm = "at+creg?\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+CREG: 1,6\r\n",
				.rsp_handler = NULL,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = NULL,
		.comm_timeout = 100
	},
	[NET_REG_URC_SET] = {
		.comm = "at+creg=1\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = NULL,
		.comm_timeout = 100
	},
	[MNO_CONF_QUERY] = {
		.comm = "at+umnoconf?\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+UMNOCONF: "MODEM_UMNOCONF_VAL",0\r\n",
				.rsp_handler = NULL,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = NULL,
		.comm_timeout = 100
	},
	[MNO_CONF_SET] = {
		.comm = "at+umnoconf="MODEM_UMNOCONF_VAL"\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = NULL,
		.comm_timeout = 190000
	}
};

#endif
