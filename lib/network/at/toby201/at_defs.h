/**
 * \file at_defs.h
 *
 * \brief AT command table definations for ublox-toby201 LTE modem functions
 *
 * \copyright Copyright (C) 2016, 2017 Verizon. All rights reserved.
 *
 *
 */

#ifndef AT_TOBY_H
#define AT_TOBY_H

#include <stdint.h>
#include <stdbool.h>
#include "uart.h"

/* AT layer internal state machine */
typedef enum at_states {
        IDLE = 1,
        /* Indicates command has been issued and now waiting for the response */
        WAITING_RESP = 1 << 1,
        /* Network lost indication from cereg and ureg */
        NETWORK_LOST = 1 << 2,
        /* TCP successfully connected */
        TCP_CONNECTED = 1 << 3,
        /* TCP closed gracefully */
        TCP_CONN_CLOSED = 1 << 4,
        /* AT layer now processing response */
        PROC_RSP = 1 << 5,
        /* AT layer processing urc outside of the interrupt context or callback
         * from uart layer
         */
        PROC_URC = 1 << 6,
        /* Connected in Direct link (DL) mode which makes socket to UART
         * connection transperent
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

typedef enum at_return_codes {
        AT_SUCCESS = 0,
        AT_RSP_TIMEOUT,
        AT_FAILURE,
        AT_TX_FAILURE,
        AT_TCP_FAILURE,
        AT_WRONG_RSP,
        AT_RECHECK_MODEM
} at_ret_code;


/* Enable this macro to display messages, error will alway be reported if this
 * macro is enabled while V2 and V1 will depend on debug_level setting
 */
/*#define DEBUG_AT_LIB*/

static int debug_level;
/* level v2 is normally for extensive debugging need, for example tracing
 * function calls
 */
#ifdef DEBUG_AT_LIB
#define DEBUG_V2(...)	\
                        do { \
                                if (debug_level >= 2) \
                                        printf(__VA_ARGS__); \
                        } while (0)

/* V1 is normaly used for variables, states which are internal to functions */
#define DEBUG_V1(...) \
                        do { \
                                if (debug_level >= 1) \
                                        printf(__VA_ARGS__); \
                        } while (0)

#define DEBUG_V0(...)	printf(__VA_ARGS__)
#else
#define DEBUG_V0(...)
#define DEBUG_V1(...)
#define DEBUG_V2(...)

#endif

#define DBG_STATE
#ifdef DBG_STATE
#define DEBUG_STATE(...) printf("%s: line %d, state: %u\n",\
                                __func__, __LINE__, (uint32_t)state)
#else
#define DEBUG_STATE(...)
#endif

/* Enable to debug wrong response, this prints expected vs received buffer in
 * raw format
 */
/*#define DEBUG_WRONG_RSP*/

#define CHECK_NULL(x, y) do { \
                                if (!((x))) { \
                                        printf("Fail at line: %d\n", __LINE__); \
                                        return ((y)); \
                                } \
                         } while (0);

#define CHECK_SUCCESS(x, y, z)	\
                        do { \
                                if ((x) != (y)) { \
                                        printf("Fail at line: %d\n", __LINE__); \
                                        return (z); \
                                } \
                        } while (0);

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define IDLE_CHARS	        10
#define MODEM_RESET_DELAY       25000 /* In mili seconds */
/* in mili seconds, polling for modem */
#define CHECK_MODEM_DELAY    1000
/* maximum timeout value in searching for the network coverage */
#define NET_REG_CHECK_DELAY     60000 /* In mili seconds */

/* Waiting arbitrarily as we do not know for how many
 * bytes we will be waiting for when modem does not send wrong response in
 * totality
 */
#define RSP_BUF_DELAY        2000 /* In mili seconds */

/* It will be used when partial matched bytes are the only left to read out */
#define DL_PARTIAL_WAIT      1000 /* In mili seconds */

/* Error codes just to process partial escape or disconnect from dl mode
 * sequence when tcp receive is being called
 */
#define DL_PARTIAL_TO_FULL      -4
#define DL_PARTIAL_ERROR        -5
#define DL_PARTIAL_SUC          0

/* Intermediate buffer to hold data from uart buffer when disconnect string
 * is detected fully, disconnect string is from the dl mode
 */
typedef struct _at_intr_buf {
        /* This buffer does not overflow as it is only being filled when whole
         * dl disconnect string sequence is detected, total capacity of this
         * buffer can not exceed beyond UART_RX_BUFFER_SIZE - disconnect string
         */
        uint8_t buf[UART_RX_BUFFER_SIZE];
        buf_sz ridx;
        buf_sz buf_unread;
} at_intr_buf;

#define MAX_RSP_LINE            2 /* Some command send response plus OK */

/** response descriptor */
typedef struct _at_rsp_desc {
        /** Hard coded expected response for the given command */
        const char *rsp;
        /** Response handler if any */
        void (*rsp_handler)(void *rcv_rsp, int rcv_rsp_len,
                                        const char *stored_rsp, void *data);
        /** private data */
        void *data;
} at_rsp_desc;

/** Command description */
typedef struct _at_command_desc {
        /** skeleton for the command */
        const char *comm_sketch;
        /** final command after processing command skeleton */
        char *comm;
        /** command timeout, indicates bound time to receive response */
        uint32_t comm_timeout;
        at_rsp_desc rsp_desc[MAX_RSP_LINE];
        /** Expected command error */
        const char *err;
} at_command_desc;

#endif /* at_defs.h */
