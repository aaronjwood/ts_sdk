/**
 * \file at_toby201_command.h
 *
 * \brief AT command for ublox-toby201 LTE modem functions
 *
 * \copyright Copyright (C) 2016, Verizon. All rights reserved.
 *
 *
 */

#include "at_defs.h"

static void __at_parse_tcp_conf_rsp(void *rcv_rsp, int rcv_rsp_len,
                                        const char *stored_rsp, void *data);
static void __at_parse_tcp_send_rsp(void *rcv_rsp, int rcv_rsp_len,
                                        const char *stored_rsp, void *data);
static void __at_parse_read_qry_rsp(void *rcv_rsp, int rcv_rsp_len,
                                        const char *stored_rsp, void *data);
static void __at_parse_tcp_sock_stat(void *rcv_rsp, int rcv_rsp_len,
                                        const char *stored_rsp, void *data);

static const char *at_urcs[URC_END] = {
                [NET_STAT_URC] = "\r\n+CEREG: ",
                [EPS_STAT_URC] = "\r\n+UREG: ",
                [NO_CARRIER] = "\r\nNO CARRIER\r\n",
                [DATA_READ] = "\r\n+UUSORD: ",
                [TCP_CLOSED] = "\r\n+UUSOCL: "
};

static at_command_desc modem_net_status_comm[MOD_END] = {
        [MODEM_OK] = {
                .comm = "at\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err_desc =  {
                        {
                                .err = NULL,
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 50
        },
        [ECHO_OFF] = {
                .comm = "ate0\r",
                .rsp_desc = {
                        {
                                .rsp = "ate0\r\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 50
        },
        [MODEM_RESET] = {
                .comm = "at+cfun=16\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err_desc =  {
                        {
                                .err = NULL,
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 5000
        },
        [NET_STAT] = {
                .comm = "at+cereg=1\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err_desc =  {
                        {
                                .err = NULL,
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 50
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
                .err_desc =  {
                        {
                                .err = NULL,
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 50
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
                .err_desc =  {
                        {
                                .err = NULL,
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 50
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
                .err_desc =  {
                        {
                                .err = NULL,
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 10000
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
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 50
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
                .err_desc =  {
                        {
                                .err = NULL,
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 50
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
                .err_desc =  {
                        {
                                .err = NULL,
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 50
        }
};

static at_command_desc pdp_conf_comm[PDP_END] = {
        [SEL_IPV4_PREF] = {
                .comm = "at+upsd=0,0,2\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 50
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
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 150000
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
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
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
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 20000
        },
        [TCP_SEND] = {
                .comm_sketch = "at+usowr=%d,%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+USOWR: ",
                                .rsp_handler = __at_parse_tcp_send_rsp,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 10000
        },
        [TCP_WRITE_PROMPT] = {
                .comm_sketch = "at+usowr=%d,%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n@",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 10000
        },
        [TCP_RCV] = {
                .comm_sketch = "at+usord=%d,%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+USORD: ",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 10000
        },
        [TCP_RCV_QRY] = {
                .comm_sketch = "at+usord=%d,0\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+USORD: ",
                                .rsp_handler = __at_parse_read_qry_rsp,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 10000
        },
        [TCP_SOCK_STAT] = {
                .comm_sketch = "at+usoctl=%d,10\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+USOCTL: ",
                                .rsp_handler = __at_parse_tcp_sock_stat,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 20
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
                .err_desc =  {
                        {
                                .err = "\r\nERROR\r\n",
                                .err_handler = NULL,
                                .data = NULL,
                        }
                },
                .comm_timeout = 10000
        }
};
