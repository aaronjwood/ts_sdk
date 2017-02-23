/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>
#include <stdbool.h>
#include "cloud_comm.h"
#include "service_ids.h"
#include "service_common.h"
#include "cc_basic_service.h"
#include "dbg.h"

/*
 * This is the implementation of the Basic service which allows the application
 * and cloud to exchange variable length binary blobs.
 */

struct __attribute__((packed)) basic_header {
	uint8_t version;		/* Version of control service protocol*/
};

/*
 * This callback is invoked when the protocol stack dispatches an event
 * associated with this service's service id.
 */
static void basic_dispatch_callback(cc_buffer_desc *buf, cc_event event,
				    cc_service_id svc_id,
				    cc_svc_callback_rtn cb)
{
	bool s;

	if (svc_id != CC_SERVICE_BASIC) {
		dbg_printf("%s:%d: Callback called for wrong svc_id: %d\n",
			   __func__, __LINE__, svc_id);
		return;
	}

	cc_buffer_desc tmp_desc = *buf;

	if (event == CC_EVT_RCVD_MSG) {
		/* XXX Basic service messages are supposed to have a header,
		   except OTT doesn't have one yet.  Will need to cheat
		   here until OTT is brought into sync. */
#if defined(OTT_PROTOCOL)
		   s = cc_adjust_buffer_desc(buf, +0);
#else
		   s = cc_adjust_buffer_desc(buf, sizeof(struct basic_header));
#endif
		   if (!s) {
			   dbg_printf("Invalid length for in Basic service "
				      "message\n");
			   return;
		   }
	}
	cb(event, 0, (void *)buf);
	*buf = tmp_desc;
}

const cc_service_descriptor cc_basic_service_descriptor = {
	.svc_id = CC_SERVICE_BASIC,
	.dispatch_callback = basic_dispatch_callback
};
