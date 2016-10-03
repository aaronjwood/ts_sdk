/**
 * \file at_toby201_command.h
 *
 * \brief AT command for ublox-toby201 LTE modem functions
 *
 * \copyright Copyright (C) 2016, Verizon. All rights reserved.
 *
 *
 */

#include "at_toby201.h"

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
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error =  NULL,
                .comm_timeout = 20
        },
        [ECHO_OFF] = {
                .comm = "ate0\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error =  "\r\nERROR\r\n",
                .comm_timeout = 20
        },
        [NET_STAT] = {
                .comm = "at+cereg=1\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = NULL,
                .comm_timeout = 20
        },
        [EPS_STAT] = {
                .comm = "at+ureg=1\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = NULL,
                .comm_timeout = 20
        },
        [MNO_STAT] = {
                .comm = "at+umnoconf?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+UMNOCONF: 3,23\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = NULL,
                .comm_timeout = 100
        },
        [MNO_SET] = {
                .comm = "at+umnoconf=3,23\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+UMNOCONF: 3,23\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = NULL,
                .comm_timeout = 100
        },
        [SIM_READY] = {
                .comm = "at+cpin?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+CPIN: READY\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 100
        },
        [NET_REG_STAT] = {
                .comm = "at+cereg?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+CEREG: 1,1\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = NULL,
                .comm_timeout = 20
        },
        [EPS_REG_STAT] = {
                .comm = "at+ureg?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+UREG: 1,7\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = NULL,
                .comm_timeout = 20
        }
};

static at_command_desc pdp_conf_comm[PDP_END] = {
        [SEL_IPV4_PREF] = {
                .comm = "at+upsd=0,0,2\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 20
        },
        [ACT_PDP] = {
                .comm = "at+upsda=0,3\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        },
                        {
                                .rsp = "\r\n+UUPSDA: 0,",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 150000
        }
};

static at_command_desc tcp_comm[TCP_END] = {
        [TCP_CONF] = {
                .comm = "at+usocr=6\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+USOCR: ",
                                .parse_rsp = TCP_CONF,
                                .data = -1
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 20
        },
        [TCP_CONN] = {
                .comm_sketch = "at+usoco=%d,%s,%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 20000
        },
        [TCP_SEND] = {
                .comm_sketch = "at+usowr=%d,%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+USOWR: ",
                                .parse_rsp = TCP_SEND,
                                .data = -1
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 10000
        },
        [TCP_WRITE_PROMPT] = {
                .comm_sketch = "at+usowr=%d,%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n@",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 20
        },
        [TCP_RCV] = {
                .comm_sketch = "at+usord=%d,%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+USORD: ",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 10000
        },
        [TCP_RCV_QRY] = {
                .comm_sketch = "at+usord=%d,0\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+USORD: ",
                                .parse_rsp = TCP_RCV_QRY,
                                .data = -1
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 10000
        },
        [TCP_SOCK_STAT] = {
                .comm_sketch = "at+usoctl=%d,10\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+USOCTL: ",
                                .parse_rsp = TCP_SOCK_STAT,
                                .data = -1
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 20
        },
        [TCP_CLOSE] = {
                .comm_sketch = "at+usocl=%d\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .parse_rsp = -1,
                                .data = -1
                        }
                },
                .error = "\r\nERROR\r\n",
                .comm_timeout = 10000
        }
};
