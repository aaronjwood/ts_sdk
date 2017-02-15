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

/* All services must define a descriptor with service id and entry points */
struct cc_service_descriptor {
	cc_service_id svc_id;
	cc_service_dispatch_callback dispatch_callback;
};

#endif
