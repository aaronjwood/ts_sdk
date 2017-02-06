/**
 * \file at_tcp_defs.h
 *
 * \brief AT state definitions for ublox-toby201 to communicate over TCP
 *
 * \copyright Copyright (C) 2016, 2017 Verizon. All rights reserved.
 */

#ifndef AT_TCP_DEFS_H
#define AT_TCP_DEFS_H

#include "at_core.h"

/* AT layer internal state machine for TCP */
typedef enum at_states {
        IDLE = 1,
        /* Network lost indication from cereg and ureg */
        NETWORK_LOST = 1 << 2,
        /* TCP successfully connected */
        TCP_CONNECTED = 1 << 3,
        /* TCP closed gracefully */
        TCP_CONN_CLOSED = 1 << 4,
        /* Connected in Direct link (DL) mode which makes socket to UART
         * connection transparent
         */
        DL_MODE = 1 << 7,
        /* Processing TCP receive call at the moment */
        TCP_DL_RX = 1 << 8,
        /* remote side disconnected, to handle this scenario in DL mode, new set
         * of disconnect state machine is being used as below
         */
        TCP_REMOTE_DISCONN = 1 << 9,
        AT_INVALID = 1 << 10
} at_states;

/* state machine just to process abrupt remote disconnect sequence in dl mode
 */
typedef enum dis_states {
        DIS_IDLE = 1,
        /* disconnect or dl mode escape sequence is partially received */
        DIS_PARTIAL = 2,
        DIS_FULL = 3
} dis_states;

/* It will be used when partial matched bytes are the only left to read out */
#define DL_PARTIAL_WAIT      1000 /* In mili seconds */

/* Error codes just to process partial escape or disconnect from dl mode
 * sequence when tcp receive is being called
 */
#define DL_PARTIAL_TO_FULL      -4
#define DL_PARTIAL_ERROR        -5
#define DL_PARTIAL_SUC          0

#endif	/* at_tcp_defs.h */
