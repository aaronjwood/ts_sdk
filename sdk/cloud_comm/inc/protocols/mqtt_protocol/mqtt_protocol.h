/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __MQTT_PROTO
#define __MQTT_PROTO

#include <stdint.h>
#include <stdbool.h>
#include "service_ids.h"
#include "protocol_def.h"

typedef struct auth_creds {
	size_t serv_cert_len;
	const unsigned char *serv_cert;
	size_t cl_cert_len;
	const unsigned char *cl_cert;
	size_t cl_key_len;
	const unsigned char *cl_key;
} auth_creds;

/*
 * mqtt Protocol API:
 * This header lays out the mqtt Protocol API which is used to communicate with
 * the cloud. For now, there is no concept like service id for mqtt.
 */

 /*
  * Initialize the mqtt Protocol module. This will initialize the underlying modem
  * /TCP drivers, set up the TLS parameters and initialize any certificates, if
  * needed. This must be called once before using any of the APIs in this module.
  *
  * Parameters:
  * 	None
  *
  * Returns:
  * 	PROTO_OK    : The API and underlying drivers were properly initialized.
  * 	PROTO_ERROR : Initialization failed.
  */
proto_result mqtt_protocol_init(void);

/*
 * Initialize the mqtt Protocol module with device authorization credential which
 * will be needed in all future communications with the cloud serivices.
 * Parameters:
 * 	creds : Pointer to a device/client, server certificate and device
 *              private keys.
 *
 * Returns:
 * 	PROTO_OK    : Setting authorization credentials was successful.
 * 	PROTO_INV_PARAM : Passed parameters are not valid.
 * Note: This API must be called before any communication with the cloud.
 */
proto_result mqtt_set_auth(const auth_creds *creds);

/*
 * Retrieves polling interval in milliseconds
 *
 * Returns:
 * 	Interval time in miliseconds
 */
uint32_t mqtt_get_polling_interval(void);

/*
 * Set the polling interval in mili seconds
 *
 * Parameters:
 * 	interval	: Polling interval in miliseconds
 */
void mqtt_set_polling_interval(uint32_t interval_ms);

/*
 * Initialize the protocol module with the remote host and port
 * Parameters:
 * 	dest : A NULL terminated string specifying destination for the
 *             connection to the cloud.
 *
 * The string has the form:  hostname:port if port is specified
 *
 * Returns:
 * 	PROTO_OK : Setting host and port was successful.
 * 	PROTO_INV_PARAM : Passed parameters are not valid.
 *
 * Note: This must be called at least once before attempting to send messages.
 */
proto_result mqtt_set_destination(const char *dest);

/*
 * Setting receiving buffer where all the incoming data will be stored and
 * callback to indicate received data to upper level
 * Parameters:
 * 	rcv_buf : Pointer to a buffer.
 * 	sz : Size of buffer.
 *	rcv_cb : callback on receiving message
 * Returns:
 * 	PROTO_OK    : Setting receive buffer was successful.
 * 	PROTO_INV_PARAM : Passed parameters are not valid.
 * Note: This must be called at least once before attempting to communicate with
 *       the cloud.
 */
proto_result mqtt_set_recv_buffer_cb(void *rcv_buf, uint32_t sz,
				proto_callback rcv_cb);

/*
 * Sends a message to the cloud service. This call is blocking in nature.
 *
 * Parameters:
 *	buf    : Message to send.
 *	sz     : Size of the message in bytes.
 *      svc_id : this parameter will be ignored.
 * 	cb     : Callback to indicate the event related to send activity.
 *
 * Returns:
 * 	PROTO_OK        : message was sent to the cloud.
 * 	PROTO_INV_PARAM : Invalid parameters
 * 	PROTO_ERROR     : Sending the message failed.
 * 	PROTO_TIMEOUT   : Timed out sending the message. Sending failed.
 */
proto_result mqtt_send_msg_to_cloud(const void *buf, uint32_t sz,
				   proto_service_id svc_id, proto_callback cb);

/*
 * Maintenance of the protocol which can be used to complete any
 * pending internal protocol activities before upper level possibly Application
 * decides to sleep for example or relinquish protocol control.
 *
 * Parameters:
 *
 */
void mqtt_maintenance(void);

/*
 * Close the connection
 *
 * Parameters:
 *
 */
void mqtt_initiate_quit();

/*
 * Retrieve the binary data pointer from the received message
 *
 * Parameters:
 * 	msg : Starting pointer to received buffer.
 *
 * Returns:
 * 	data pointer or NULL if fails.
 */
const uint8_t *mqtt_get_rcv_buffer_ptr(const void *msg);

#endif
