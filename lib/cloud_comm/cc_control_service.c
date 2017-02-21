/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>
#include <stdbool.h>
#include "cloud_comm.h"
#include "service_ids.h"
#include "service_common.h"
#include "cloud_protocol_intfc.h"
#include "dbg.h"

/*
 * This is the implementation of the Control service which is automatically
 * registered in all applications.  This service provides for configuration
 * and control of the protocol stack, and for delivering pre-defined events
 * which all applications are expected to handle.
 */

CC_SEND_BUFFER(control_send_buf, CC_MIN_SEND_BUF_SZ);

struct __attribute__((packed)) control_header {
	uint8_t version;		/* Version of control service protocol*/
	uint8_t msg_type;		/* Control service message type */
};

static inline uint32_t load_uint32_le(uint8_t *src)
{
	return (src[3] << 24) | (src[2] << 16) | (src[1] << 8) | src[0];
}

/*
 * This callback is invoked when the application dispatches an event
 * associated with this service's service id.
 */
static void control_dispatch_callback(cc_buffer_desc *buf, cc_event event,
				      cc_service_id svc_id,
				      cc_svc_callback_rtn cb)
{
	struct control_header *hdr;
	
	if (svc_id != CC_SERVICE_CONTROL) {
		dbg_printf("%s:%d: Callback called for wrong svc_id: %d\n",
			   __func__, __LINE__, svc_id);
		return;
	}

	/* Events for sent messages */
	if (event == CC_EVT_SEND_ACKED) {
		return;
	} else if (event == CC_EVT_SEND_NACKED) {
		dbg_printf("Sending Control service message failed\n");
		return;
	} else if (event == CC_EVT_SEND_TIMEOUT) {
		dbg_printf("Timeout while sending Control service message\n");
		return;
	} else if (event != CC_EVT_RCVD_MSG) {
		dbg_printf("%s:%d: Dispatched an unsupported event: %d\n",
			   __func__, __LINE__, event);
		return;
	}

	/* Process a received Control message */
	hdr = (struct control_header *)cc_get_recv_buffer_ptr(buf);
	if (hdr->version != CONTROL_PROTOCOL_VERSION) {
		dbg_printf("%s:%d: Unsupported Control protocol version: %d\n",
			   __func__, __LINE__, event);
		goto bad_msg;
	}

	switch (hdr->msg_type) {

	case CTRL_MSG_MAKE_DEVICE_SLEEP: {
		/*
		 * Message contains 4 byte sleep time in seconds.
		 * Deliver it as an event to the application's control callback.
		 */
		uint8_t *body = (uint8_t *)hdr + sizeof(struct control_header);
		uint32_t sleep_interval = load_uint32_le(body);
		/* XXX Should we sanity check the sleep interval? */
		cb(CC_EVT_CTRL_SLEEP, sleep_interval, NULL);
		break;
		}

	case CTRL_MSG_SET_POLLING_INTERVAL: {
		/*
		 * Message contains 4 byte polling interval in seconds.
		 * Pass the value to the protocol layer.
		 * No need to involve the application.
		 */
		uint8_t *body = (uint8_t *)hdr + sizeof(struct control_header);
		uint32_t polling_interval = load_uint32_le(body);
		PROTO_SET_POLLING(1000 * polling_interval); /* milliseconds */
		break;
		}

	default:
		dbg_printf("%s:%d: Unsupported Control message type: %d\n",
			   __func__, __LINE__, hdr->msg_type);
		goto bad_msg;
	}

	cc_ack_msg();
	return;

bad_msg:
	cc_nak_msg();
}

cc_send_result cc_ctrl_resend_init_config(cc_callback_rtn cb)
{
	struct control_header *hdr;
	
	hdr = (struct control_header *)cc_get_send_buffer_ptr(&control_send_buf);
	hdr->version = CONTROL_PROTOCOL_VERSION;
	hdr->msg_type = CTRL_MSG_REQUEST_RESEND_INIT;

	return cc_send_svc_msg_to_cloud(&control_send_buf, sizeof(*hdr),
					CC_SERVICE_CONTROL, cb);
}

cc_service_descriptor cc_control_service_descriptor = {
	.svc_id = CC_SERVICE_CONTROL,
	.dispatch_callback = control_dispatch_callback
};
