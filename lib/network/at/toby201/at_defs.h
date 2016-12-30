/**
 * \file at_defs.h
 *
 * \brief AT command table definations for ublox-toby201 LTE modem functions
 *
 * \copyright Copyright (C) 2016, Verizon. All rights reserved.
 *
 *
 */

#ifndef AT_TOBY_H
#define AT_TOBY_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_RSP_LINE            2 /* Some command send response plus OK */

/* Maximum modem tx buffer size, this limit is imposed by modem internal
 * tcp transmit buffer
 */
#define MAX_AT_TCP_TX_SIZE      1024

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
