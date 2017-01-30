/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __OTT_PRIV
#define __OTT_PRIV

#include <stdint.h>
#include <stdbool.h>
#include "cloud_comm.h"

static struct {
	bool send_in_progress;		/* Set if a message is currently being sent */
	const cc_buffer_desc *buf;	/* Outgoing data buffer */
	cc_callback_rtn cb;		/* "Send" callback */
} conn_out;

static struct {
	bool recv_in_progress;		/* Set if a receive was scheduled */
	cc_buffer_desc *buf;		/* Incoming data buffer */
	cc_callback_rtn cb;		/* "Receive" callback */
} conn_in;

static struct {
	uint32_t start_ts;		/* Polling interval measurement starts
					 * from this timestamp */
	uint32_t polling_int_ms;	/* Polling interval in milliseconds */
	bool new_interval_set;		/* Set if a new interval was received */
} timekeep;

#endif
