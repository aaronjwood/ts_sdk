/**
 * \file at_toby201_tcp_command.h
 *
 * \brief AT commands for the ublox-toby201 modem to communicate over TCP
 *
 * \copyright Copyright (C) 2016, 2017 Verizon. All rights reserved.
 *
 *
 */
#ifndef AT_TOBY_TCP_COMM_H
#define AT_TOBY_TCP_COMM_H

#include "at_tcp_defs.h"
#include "ts_sdk_modem_config.h"

/* Upper limit for commands which need formatting before sending to modem */
#define TEMP_COMM_LIMIT            64

/* Time interval between sending escape sequence characters to get out from
 * direct link mode, time interval is in fiftieth of second, configure it for
 * 100ms, minimum is 20mS
 */
#define DL_MODE_ESC_TIME           5
#define DL_ESC_TIME_MS             (DL_MODE_ESC_TIME * 20)

static void __at_parse_tcp_conf_rsp(void *rcv_rsp, int rcv_rsp_len,
                                        const char *stored_rsp, void *data);

/** Unsolicited result codes */
typedef enum at_urc {
        TCP_CLOSED, /** TCP close */
        PDP_DEACT, /** PDP connection is deactivated by network */
        DISCONNECT, /** disconnect from direct link mode */
        URC_END
} at_urc;

/** For modem status check and configuration related commands */
typedef enum at_modem_stat_command {
        MNO_STAT, /** check mobile network opertor */
        MNO_SET, /** set mobile network opertor */
        MOD_END
} at_modem_stat_command;

/** For PDN (packet data network) activation commands */
typedef enum at_pdp_command {
#if SIM_TYPE == M2M
	ADD_PDP_CTX,		/** Add PDP context */
	ACT_PDP_CTX,		/** Activate PDP Context */
	MAP_PDP_PROFILE,	/** Map UPSD profile to PDP Context */
#endif
	SEL_IPV4,		/** IPV4 stack */
	ACT_PDP_PROFILE,	/** Activate PDP context with specific profile */
	PDP_END
} at_pdp_command;

/** TCP related commands */
typedef enum at_tcp_command {
        TCP_CONF = 0, /** TCP connection configuration */
        TCP_CONN, /** TCP connection */
        TCP_CLOSE,
        TCP_DL_MODE,
        DL_CONG_CONF,
        ESCAPE_DL_MODE,
        ESCAPE_TIME_CONF,
        TCP_END
} at_tcp_command;


static const char *at_urcs[URC_END] = {
                [TCP_CLOSED] = "\r\n+UUSOCL: ",
                [PDP_DEACT] = "\r\n+UUPSDD: ",
                [DISCONNECT] = "\r\nDISCONNECT\r\n"
};

static const at_command_desc modem_net_status_comm[MOD_END] = {
        [MNO_STAT] = {
                .comm = "at+umnoconf?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+UMNOCONF: "MODEM_UMNOCONF_VAL"\r\n",
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
        [MNO_SET] = {
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

static const at_command_desc pdp_conf_comm[PDP_END] = {
#if SIM_TYPE == M2M
        [ADD_PDP_CTX] = {
                .comm = "at+cgdcont="MODEM_APN_CTX_ID",\""MODEM_APN_TYPE"\",\""\
			 MODEM_APN_VALUE"\"\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 100
        },
        [ACT_PDP_CTX] = {
                .comm = "at+cgact=1,"MODEM_APN_CTX_ID"\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 500
        },
        [MAP_PDP_PROFILE] = {
                .comm = "at+upsd=0,100,"MODEM_APN_CTX_ID"\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 100
        },
#endif
        [SEL_IPV4] = {
                .comm = "at+upsd=0,0,"MODEM_APN_TYPE_ID"\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 100
        },
        [ACT_PDP_PROFILE] = {
                .comm = "at+upsda=0,3\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\n+UUPSDA: 0,",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 190000
        }
};

static at_command_desc tcp_comm[TCP_END] = {
        [TCP_CONF] = {
                .comm = "at+usocr=6\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+USOCR: ",
                                .rsp_handler = __at_parse_tcp_conf_rsp,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 100
        },
        [TCP_CONN] = {
                .comm_sketch = "at+usoco=%d,%s,%s\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 25000
        },
        [TCP_CLOSE] = {
                .comm_sketch = "at+usocl=%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 15000
        },
        [TCP_DL_MODE] = {
                .comm_sketch = "at+usodl=%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nCONNECT\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        },
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 15000
        },
        [ESCAPE_DL_MODE] = {
                .comm = "+++",
                .rsp_desc = {
                        {
                                .rsp = "\r\nDISCONNECT\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 2000
        },
        [ESCAPE_TIME_CONF] = {
                .comm_sketch = "ats12=%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        },
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 100
        },
        [DL_CONG_CONF] = {
                .comm_sketch = "at+udconf=8,%d,0\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        },
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 100
        }
};
#endif /* at_toby201_tcp_command.h */
