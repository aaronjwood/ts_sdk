/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#ifndef __CC_CONTROL_SERVICE_H
#define __CC_CONTROL_SERVICE_H

#include <stdint.h>
#include <stdbool.h>

#include "cloud_comm.h"

/**
 * \file cc_control_service.h
 *
 * This module defines a service which provides control and management
 * functions for cloud communications.  Messages for this service id
 * (CC_SERVICE_CONTROL) may be handled transparently by the service, or may
 * result in events being sent to the application's callback for the
 * Control service. 
 *
 * The service also defines a set of API functions which allow the
 * application to make service-specific requests of the cloud.
 */

extern cc_service_descriptor cc_control_service_descriptor;

/*
 * API functions to request the cloud to perform some Control service action.
 */

/**
 * \brief
 * Ask the cloud to send the initial configuration of the device.
 *
 * This function is used when the device has undergone a reset and is no
 * longer in the state the cloud services thinks it is in. A successful call
 * will result in the cloud sending messages that will restore cloud
 * communications parameters to the expected state.
 *
 * XXX Need to confirm that this is expected just to send config parameters
 * managed by the Control services i.e. nothing application related.
 *
 * \returns
 * 	CC_SEND_FAILED  : Failed to send the message.
 * 	CC_SEND_BUSY    : A send is in progress.
 * 	CC_SEND_SUCCESS : Message was sent, waiting for response from the cloud.
 */
cc_send_result cc_ctrl_resend_init_config(cc_callback_rtn cb);

#endif /* CC_CONTROL_SERVICE_H */
