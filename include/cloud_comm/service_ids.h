/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __SERVICE_IDS_H
#define __SERVICE_IDS_H

enum cc_service_number {
	CC_SERVICE_CONTROL,	/**< The device Control service */
	CC_SERVICE_BASIC,	/**< Basic service for application data */
};

/* Message type values for the Control service. */
enum cc_control_svc_message_types {
	CTRL_MSG_MAKE_DEVICE_SLEEP = 1,
	CTRL_MSG_SET_POLLING_INTERVAL = 2,
	CTRL_MSG_REQUEST_RESEND_INIT = 3,
};

#define CONTROL_SERVICE_MAX_SEND_SZ 2
#define CONTROL_SERVICE_MAX_RECV_SZ 6

#endif
