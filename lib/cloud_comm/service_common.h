/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __SERVICE_COMMON_H
#define __SERVICE_COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include "cloud_comm.h"

/* Common definitions needed by modules which implement a service */

/* All services must define an entry point to receive dispatched events. */
typedef void (*cc_service_dispatch_callback)(cc_buffer_desc *buf,
					     cc_event event,
					     cc_service_id svc_id,
					     cc_svc_callback_rtn cb);

typedef bool (*cc_send_hdr_rtn)(cc_buffer_desc *buf);

/* All services must define a descriptor with service id and entry points */
struct cc_service_descriptor {
	cc_service_id svc_id;
	uint8_t send_offset;
	uint8_t recv_offset;
	cc_service_dispatch_callback dispatch_callback;
	cc_send_hdr_rtn add_send_hdr;
};

#endif
