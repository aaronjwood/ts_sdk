/* Copyright (C) 2017 Verizon. All rights reserved. */

#ifndef AT_TOBY_SMS_COMMAND_H
#define AT_TOBY_SMS_COMMAND_H

#include "at_core.h"

enum at_modem_network_commands {
	IMS_REG_QUERY,
	IMS_REG_URC_SET,
	MNO_CONF_QUERY,
	MNO_CONF_SET,
	AUTO_TIME_ZONE_QUERY,
	AUTO_TIME_ZONE_SET,
	NUM_MODEM_COMMANDS
};

enum at_modem_sms_commands {
	SMS_SET_SMS_FORMAT_3GPP,
	SMS_ENTER_PDU_MODE,
	SMS_SEND,
	SMS_SET_CNMI,
	SMS_SEND_ACK,
	SMS_SEND_NACK,
};

typedef enum at_urc {
	IMS_STAT_URC,
	UCMT_URC,
	NUM_URCS
} at_urc;

static const char *at_urcs[NUM_URCS] = {
	[IMS_STAT_URC] = "\r\n+CIREGU: ",
	[UCMT_URC] = "\r\n+UCMT: "
};

static const at_command_desc mod_netw_cmd[NUM_MODEM_COMMANDS] = {
        [IMS_REG_QUERY] = {
                .comm = "at+cireg?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+CIREG: 1,1\r\n",
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
        [IMS_REG_URC_SET] = {
                .comm = "at+cireg=1\r",
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
                                .rsp = "\r\n+UMNOCONF: 3,7\r\n",
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
                .comm = "at+umnoconf=3,7\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 190000
        },
	[AUTO_TIME_ZONE_QUERY] = {
		.comm = "at+ctzu?\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+CTZU: 1\r\n",
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
	[AUTO_TIME_ZONE_SET] = {
		.comm = "at+ctzu=1\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = NULL,
		.comm_timeout = 100
	}
};

#endif
