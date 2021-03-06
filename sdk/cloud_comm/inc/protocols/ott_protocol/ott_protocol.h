/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#ifndef __OTT_PROTO
#define __OTT_PROTO

#include <stdint.h>
#include <stdbool.h>
#include "service_ids.h"
#include "protocol_def.h"

/*
 * OTT Protocol API:
 * This header lays out the OTT Protocol API which is used to communicate with
 * the cloud. The cloud never initiates a connection on its own. Instead, the
 * device polls the cloud at specified intervals and whenever it sends data, to
 * read any messages back from the cloud.
 * There is never more than one outstanding message in this protocol.
 * The device reports its status to the cloud at another fixed interval.
 */

 /*
  * Initialize the OTT Protocol module. This will initialize the underlying modem
  * / TCP drivers, set up the TLS parameters and initialize any certificates, if
  * needed. This must be called once before using any of the APIs in this module.
  *
  * Parameters:
  * 	None
  *
  * Returns:
  * 	PROTO_OK    : The API and underlying drivers were properly initialized.
  * 	PROTO_ERROR : Initialization failed.
  */
proto_result ott_protocol_init(void);

/*
 * Initialize the OTT Protocol module with device authorization credential which
 * will be needed in all future communications with the cloud serivices.
 * Parameters:
 * 	d_id     : Pointer to a device ID.
 *      d_id_sz  : Size in bytes of d_id
 *      d_sec    : Pointer to the buffer holding the device secret.
 * 	d_sec_sz : Size of the device secret in bytes.
 *
 *
 * Returns:
 * 	PROTO_OK    : Setting authorization credentials was successful.
 * 	PROTO_INV_PARAM : Passed parameters are not valid.
 * Note: This API must be called before any communication with the cloud.
 */
proto_result ott_set_own_auth(const uint8_t *d_id, uint32_t d_id_sz,
        const uint8_t *d_sec, uint32_t d_sec_sz);

/*
 * Initialize the OTT Protocol module with root ca credential which
 * will be needed in all future communications with the cloud serivices to verify
 * remote endpoint.
 * Parameters:
 * 	serv_cert : Pointer to a server certificate.
 *      cert_len  : Size in bytes of serv_cert
 *
 *
 * Returns:
 * 	PROTO_OK    : Setting authorization credentials was successful.
 * 	PROTO_INV_PARAM : Passed parameters are not valid.
 * Note: This API must be called before any communication with the cloud.
 */
proto_result ott_set_remote_auth(const uint8_t *serv_cert, uint32_t cert_len);

/*
 * Retrieves polling interval in milliseconds
 *
 * Returns:
 * 	Interval time in miliseconds
 */
uint32_t ott_get_polling_interval(void);

/*
 * Set the polling interval in mili seconds
 *
 * Parameters:
 * 	interval	: Polling interval in miliseconds
 */
void ott_set_polling_interval(uint32_t interval_ms);

/*
 * Initialize the OTT Protocol module with the remote host and port
 * Parameters:
 * 	dest : A NULL terminated string specifying destination for the
 *             connection to the cloud.
 *
 * The string has the form:  hostname:port
 *
 * Returns:
 * 	PROTO_OK : Setting host and port was successful.
 * 	PROTO_INV_PARAM : Passed parameters are not valid.
 *
 * Note: This must be called at least once before attempting to send messages.
 */
proto_result ott_set_destination(const char *dest);

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
 */
proto_result ott_set_recv_buffer_cb(void *rcv_buf, uint32_t sz,
				proto_callback rcv_cb);

/*
 * Sends a message to the cloud service. The message is successfully
 * delivered if this call exits with the received response has the
 * ACK flag set. This call is blocking in nature.
 *
 * Parameters:
 *	buf : Message to send.
 *	sz : Size of the message in bytes.
 * 	cb : Callback to indicate the event related to send activity.
 *
 * Returns:
 * 	PROTO_OK        : message was sent to the cloud.
 * 	PROTO_INV_PARAM : Invalid parameters
 * 	PROTO_ERROR     : Sending the message failed.
 * 	PROTO_TIMEOUT   : Timed out sending the message. Sending failed.
 */
proto_result ott_send_msg_to_cloud(const void *buf, uint32_t sz,
				   proto_service_id svc_id, proto_callback cb);

/*
 * Maintenance of the protocol which can be used to complete any
 * pending internal protocol activities before upper level possibly Application
 * decides to sleep for example or relinquish protocol control.
 * Beside maintenance, it also includes reaching out to
 * cloud if polling is due to retrieve messages if any
 * Parameters:
 * 	poll_due: True if polling of the cloud is require
 */
void ott_maintenance(bool poll_due);

/*
 * Send a QUIT or NACK + QUIT and then close the connection and reset the state
 *
 * Parameters:
 *	send_nack : true if want to send nack before quitting connection.
 */
void ott_initiate_quit(bool send_nack);

/*
 * Send positive acknowledgment to cloud
 *
 * Parameters:
 *
 *
 * Returns:
 *
 */
void ott_send_ack(void);

/*
 * Send negative acknowledgment to cloud and also notify it about quitting
 * current connection.
 *
 * Parameters:
 *
 *
 * Returns:
 *
 */
void ott_send_nack(void);

/*
 * Retrieve the binary data pointer from the received message
 *
 * Parameters:
 * 	msg : Starting pointer to received buffer.
 *
 * Returns:
 * 	data pointer or NULL if fails.
 */
const uint8_t *ott_get_rcv_buffer_ptr(const void *msg);

/*
 * Notify the cloud service that the device has restarted and needs to retrieve
 * its initial data. A restart could be a result of a number of things, such as
 * a watch dog timeout, power glitch etc.
 * This message type does have a response.
 * This call is blocking in nature.
 *
 * Parameters:
 * 	cb : callback to indicate send status.
 *
 * Returns:
 * 	PROTO_OK      : Message was sent to the cloud service.
 * 	PROTO_ERROR   : Sending the message failed.
 *      PROTO_TIMEOUT   : Timed out sending the message. Sending failed.
 */
proto_result ott_resend_init_config(proto_callback cb);

/*
 * Retrieve the sleep interval from the received message
 *
 * Parameters:
 * 	msg : Pointer to received buffer.
 *
 * Returns:
 * 	sleep interval or 0 in case of invalid buffer or msg content
 */
uint32_t ott_get_sleep_interval(const void *msg);

#endif
