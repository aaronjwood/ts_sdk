/**
 * \file at_toby201_lte_command.h
 *
 * \brief AT commands for the ublox-toby201 modem to communicate over LTE
 *
 * \copyright Copyright (C) 2016, 2017 Verizon. All rights reserved.
 *
 *
 */
#ifndef AT_TOBY_LTE_COMM_H
#define AT_TOBY_LTE_COMM_H

#include "at_lte_defs.h"

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
        NET_STAT_URC = 0, /** network status */
        EPS_STAT_URC, /** data network status */
        TCP_CLOSED, /** TCP close */
        PDP_DEACT, /** PDP connection is deactivated by network */
        DISCONNECT, /** disconnect from direct link mode */
        URC_END
} at_urc;

/** For modem status check and configuration related commands */
typedef enum at_modem_stat_command {
        NET_STAT = 0, /** turn on network status urc */
        EPS_STAT, /** turn on data network status urc */
        MNO_STAT, /** check mobile network opertor */
        MNO_SET, /** set mobile network opertor */
        SIM_READY,
        NET_REG_STAT,
        EPS_REG_STAT,
        MOD_END
} at_modem_stat_command;

/** For PDN (packet data network) activation commands */
typedef enum at_pdp_command {
        SEL_IPV4_PREF = 0, /** configure PDP context for IPV4 for preference */
        ACT_PDP, /** activate pdp context */
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
                [NET_STAT_URC] = "\r\n+CEREG: ",
                [EPS_STAT_URC] = "\r\n+UREG: ",
                [TCP_CLOSED] = "\r\n+UUSOCL: ",
                [PDP_DEACT] = "\r\n+UUPSDD: ",
                [DISCONNECT] = "\r\nDISCONNECT\r\n"
};

static const at_command_desc modem_net_status_comm[MOD_END] = {
        [NET_STAT] = {
                .comm = "at+cereg=1\r",
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
        [EPS_STAT] = {
                .comm = "at+ureg=1\r",
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
        [MNO_STAT] = {
                .comm = "at+umnoconf?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+UMNOCONF: 3,23\r\n",
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
                .comm = "at+umnoconf=3,23\r",
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
        [SIM_READY] = {
                .comm = "at+cpin?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+CPIN: READY\r\n",
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
                .comm_timeout = 15000
        },
        [NET_REG_STAT] = {
                .comm = "at+cereg?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+CEREG: 1,1\r\n",
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
        [EPS_REG_STAT] = {
                .comm = "at+ureg?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+UREG: 1,7\r\n",
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
        }
};

static const at_command_desc pdp_conf_comm[PDP_END] = {
        [SEL_IPV4_PREF] = {
                .comm = "at+upsd=0,0,2\r",
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
        [ACT_PDP] = {
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
#endif /* at_toby201_lte_command.h */
