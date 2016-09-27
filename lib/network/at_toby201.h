/**
 * \file at_toby201.h
 *
 * \brief AT command for ublox-toby201 LTE modem functions
 *
 * \copyright Copyright (C) 2016, Verizon. All rights reserved.
 *
 *
 */

#ifndef AT_TOBY_H
#define AT_TOBY_H

#include <stdint.h>
#include <stdbool.h>

typedef enum at_urc {
        NET_STAT_URC = 0,
        EPS_STAT_URC,
        NO_CARRIER,
        DATA_READ,
        TCP_CLOSED,
        URC_END
} at_urc;

typedef enum at_modem_stat_command {
        MODEM_OK = 0,
        MODEM_RESET,
        NET_STAT,
        EPS_STAT,
        MNO_STAT,
        MNO_SET,
        SIM_READY,
        NET_REG_STAT,
        EPS_REG_STAT,
        MOD_END
} at_modem_stat_command;

typedef enum at_pdp_command {
        SEL_IPV4_PREF = 0,
        ACT_PDP,
        PDP_END
} at_pdp_command;

typedef enum at_tcp_command {
        TCP_CONF = 0,
        TCP_CONN,
        TCP_SEND,
        TCP_RCV,
        TCP_CLOSE,
        TCP_END
} at_tcp_command;

typedef struct _at_command_desc {
        const char *comm;
        const char *rsp;
        const char *error;
        uint32_t rsp_timeout;
} at_command_desc;

#define AT_TX_WAIT_MS           10000
#define MAX_TCP_SEND_BYTES      1024 /* For binary extended mode */
/* Upper limit for commands which need formatting before sending to modem */
#define TEMP_COMM_LIMIT         64
#define MAX_RSP_BYTES           64 /* bytes to store single line of response */

#endif /* at_toby201.h */
