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
        NET_STAT_URC,
        EPS_STAT_URC,
        NO_CARRIER,
        DATA_READ,
        TCP_CLOSED
} at_urc;

typedef enum at_modem_stat_command {
        MODEM_OK,
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
        SEL_IPV4_PREF,
        ACT_PDP,
        PDP_END
} at_pdp_command;

typedef enum at_tcp_command {
        TCP_CONF,
        TCP_CONN,
        TCP_SEND,
        TCP_RCV,
        TCP_END
} at_tcp_command;

typedef struct _at_command_desc {
        const char *comm;
        const char *rsp;
        const char *error;
        uint32_t rsp_timeout;
} at_command_desc;

#endif /* at_toby201.h */
