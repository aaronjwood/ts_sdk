/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>
#include <stdbool.h>
#include "cloud_comm.h"
#include "service_ids.h"
#include "service_common.h"
#include "cc_basic_service.h"
#include "dbg.h"

/* Only smsnas protocol supports basic services, others just fake it to make it
 * compatible with the overall implementation for now
 */

#define BASIC_PROTOCOL_VERSION 1

/*
 * This is the implementation of the Basic service which allows the application
 * and cloud to exchange variable length binary blobs.
 */

/*
 * XXX Basic service messages are supposed to have a header, except OTT/MQTT
 * doesn't have one yet.  Will need to cheat for now and do something protocol
 * specific.
 */
struct __attribute__((packed)) basic_header {
#if defined(SMSNAS_PROTOCOL)
	uint8_t version;		/* Version of control service protocol*/
#endif
};

#if defined(SMSNAS_PROTOCOL)
static bool check_validity(cc_buffer_desc *buf)
{

	const uint8_t *payload = cc_get_recv_buffer_ptr(buf, CC_SERVICE_BASIC);
	if (payload == NULL) {
		dbg_printf("Empty BASIC_SERVICE protocol msg\n");
		cc_nak_msg();
		return false;
	}
	struct basic_header *hdr =
		(struct basic_header *)(payload - sizeof(struct basic_header));

	if (hdr->version != BASIC_PROTOCOL_VERSION) {
		dbg_printf("Unsupported BASIC_SERVICE protocol version: %d\n",
			   hdr->version);
		cc_nak_msg();
		return false;
	}
	return true;
}
#endif

/*
 * This callback is invoked when the protocol stack dispatches an event
 * associated with this service's service id.
 */
static void basic_dispatch_callback(cc_buffer_desc *buf, cc_event event,
				    cc_service_id svc_id,
				    cc_svc_callback_rtn cb)
{
	if (svc_id != CC_SERVICE_BASIC) {
		dbg_printf("%s:%d: Callback called for wrong svc_id: %d\n",
			   __func__, __LINE__, svc_id);
		return;
	}

#if defined(SMSNAS_PROTOCOL)
	switch (event) {
	case CC_EVT_RCVD_MSG:
		if (!check_validity(buf))
			return;
		break;
	case CC_EVT_RCVD_OVERFLOW:
		dbg_printf("Memory overflow for Basic service message\n");
		break;
	default:
		dbg_printf("%s:%d: Dispatched an unsupported event: %d\n",
		   				__func__, __LINE__, event);
		return;
	}
#endif
	cb(event, 0, (void *)buf);
}

static bool basic_add_send_hdr(cc_buffer_desc *buf)
{
	uint8_t *payload = cc_get_send_buffer_ptr(buf, CC_SERVICE_BASIC);
	if (payload == NULL)
		return false;

#if defined(SMSNAS_PROTOCOL)
	struct basic_header *hdr = (struct basic_header *)(payload -
					     sizeof(struct basic_header));
	hdr->version = BASIC_PROTOCOL_VERSION;
#endif
	return true;
}

const cc_service_descriptor cc_basic_service_descriptor = {
	.svc_id = CC_SERVICE_BASIC,
	.send_offset = sizeof(struct basic_header),
	.recv_offset = sizeof(struct basic_header),
	.dispatch_callback = basic_dispatch_callback,
	.add_send_hdr = basic_add_send_hdr
};
