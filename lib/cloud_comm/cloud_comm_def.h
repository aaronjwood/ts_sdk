/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __CC_COMM_DEF
#define __CC_COMM_DEF

#include <stdint.h>
#include <stdbool.h>
#include "cloud_comm.h"

static struct {
	bool send_in_progress;		/* Set if a message is currently being sent */
	cc_buffer_desc *buf;		/* Outgoing data buffer */
} conn_out;

static struct {
	bool recv_in_progress;		/* Set if a receive was scheduled */
	cc_buffer_desc *buf;		/* Incoming data buffer */
} conn_in;

static struct {
	uint32_t start_ts;		/* Polling interval measurement starts
					 * from this timestamp */
	uint32_t polling_int_ms;	/* Polling interval in milliseconds */
} timekeep;

#endif
