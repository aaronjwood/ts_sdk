/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>
#include <stdbool.h>
#include "cloud_comm.h"
#include "service_ids.h"
#include "service_common.h"
#include "cloud_protocol_intfc.h"
#include "dbg.h"

#define CONTROL_PROTOCOL_VERSION 1	/* Currently supported proto version */

/* Implementation of the Control service used by all cloud_comm applications. */

CC_SEND_BUFFER(control_send_buf, CC_MIN_SEND_BUF_SZ);

struct control_header {
	uint8_t version;		/* Version of control service protocol*/
	uint8_t msg_type;		/* Control service message type */
};

static void control_dispatch_callback(cc_buffer_desc *buf, cc_event event,
				      cc_service_id svc_id,
				      cc_svc_callback_rtn cb)
{
	struct control_header *hdr;
	
	if (svc_id != CC_SERVICE_CONTROL) {
		dbg_printf("%s:%d: Callback called with wrong svc_id: %d\n",
			   svc_id, __func__, __LINE__);
		goto done;
	}

	if (event != CC_EVT_RCVD_MSG) {
		dbg_printf("%s:%d: Dispatched an unsupported event: %d\n",
			   event, __func__, __LINE__);
		goto done;
	}

	hdr = (struct control_header *)cc_get_recv_buffer_ptr(buf);
	if (hdr->version != CONTROL_PROTOCOL_VERSION) {
		dbg_printf("%s:%d: Unsupported Control protocol version: %d\n",
			   event, __func__, __LINE__);
		goto done;
	}

	switch (hdr->msg_type) {

	CTRL_MSG_MAKE_DEVICE_SLEEP: {
		/*
		 * Message contains 4 byte sleep time in seconds.
		 * Deliver it as an event to the application's control callback.
		 */
		uint8_t *body = (uint8_t *)hdr + sizeof(struct control_header);
		uint32_t sleep_interval = *(uint32_t *)body;
		cb(CC_EVT_CTRL_SLEEP, sleep_interval, NULL);
		break;
		}

	CTRL_MSG_SET_POLLING_INTERVAL: {
		/*
		 * Message contains 4 byte polling interval in seconds.
		 * Pass the value to the protocol layer.
		 * No need to involve the application.
		 */
		uint8_t *body = (uint8_t *)hdr + sizeof(struct control_header);
		uint32_t polling_interval = *(uint32_t *)body;
		PROTO_SET_POLLING(1000 * polling_interval); /* milliseconds */
		break;
		}

	default:
		dbg_printf("%s:%d: Unsupported Control message type: %d\n",
			   hdr->msg_type, __func__, __LINE__);
	}
done:
	cc_ack_msg();
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
