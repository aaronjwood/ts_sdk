/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __PROTOCOL
#define __PROTOCOL

#include <stdint.h>
#include <stdbool.h>
#include "dbg.h"
#include "protocol_def.h"


#ifdef OTT_PROTOCOL
#include "ott_limits.h"
#include "ott_protocol.h"
#elseif SMSNAS_PROTOCOL
#include "smsnas_limits.h"
#include "smsnas_protocol.h"
#else
Please define protocol to use, valid options OTT_PROTOCOL or SMSNAS_PROTOCOL
#endif

/*
 * Protocol API:
 * This header lays out the generic Protocol API which is used to communicate
 * with the cloud. APIs also provide means for the protocols which need polling
 * to cloud periodically to retriece updates if any. Also, includes API to set
 * authentication details for the protocols.
 */


/*
 * Optional API to initialize the Protocol module with device authorization
 * credential which will be needed in all future communications with the
 * cloud serivices.
 * Parameters:
 * 	d_id     : Pointer to a unique device ID.
 *      d_id_sz  : size of the unique device id.
 * 	d_sec_sz : Size of the device secret in bytes.
 * 	d_sec    : Pointer to the buffer holding the device secret.
 *
 * Returns:
 * 	PROTO_OK    : Setting authorization credentials was successful.
 * 	PROTO_INV_PARAM : Passed parameters are not valid.
 * Note: This API must be called before ott_send_auth_to_cloud API. Depending on
 * 	 the protocol used, d_id or d_sec or can be optional.
 */
proto_result proto_set_auth(const uint8_t *d_id, uint32_t d_id_sz,
                         const uint8_t *d_sec, uint32_t d_sec_sz);

/*
 * Retrieves default polling interval in mili seconds which will be used to
 * querry cloud for updates if any.
 * Returns:
 * 	Interval time in miliseconds if defined or 0
 */
uint32_t proto_get_default_polling();

/*
 * Maintenance of the protocol which can be used to complete any
 * pending internal protocol activities before upper level possibly Application
 * decides to sleep for example or relinquish protocol control.
 * Beside maintenance, it also includes reaching out to
 * cloud if polling is due to retrieve messages if any
 * Parameters:
 * 	poll_due: True if polling of the cloud is require along with protocol
 *		  maintenance
 */
void proto_maintenance(bool poll_due);

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
proto_result proto_set_recv_buffer_cb(void *rcv_buf, uint32_t sz,
					proto_callback rcv_cb);

/*
 * Initialize the Protocol module with remote host and port which
 * will be needed in all future communications with the cloud serivices.
 * Parameters:
 * 	host : A NULL terminated string specifying the host name.
 * 	port : A NULL terminated string specifying the port number.
 *
 * Returns:
 * 	PROTO_OK : Setting host and port was successful.
 * 	PROTO_INV_PARAM : Passed parameters are not valid.
 * Note: This must be called at least once before attempting to send messages.
 */
proto_result proto_set_destination(const char *host, const char *port);

/*
 * Initialize the Protocol module.
 * This must be called once before using any of the APIs in this module.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	PROTO_OK    : The API and underlying drivers were properly initialized.
 * 	PROTO_ERROR : Initialization failed.
 */
proto_result proto_init(void);

/*
 * Blocking call to sends a message to the cloud service.
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
proto_result proto_send_msg_to_cloud(void *msg, uint32_t sz, proto_callback cb);

/*
 * Send positive acknowledgment to cloud
 *
 * Parameters:
 *
 *
 * Returns:
 *
 */
void proto_send_ack(void);

/*
 * Send negative acknowledgment to cloud
 *
 * Parameters:
 *
 *
 * Returns:
 *
 */
void proto_send_nack(void);

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
proto_result proto_resend_init_config(proto_callback cb);

/*
 * Retrieve starting pointer of the binary data from the received message
 *
 * Parameters:
 * 	msg : Pointer to received buffer.
 *
 * Returns:
 * 	data pointer or NULL if fails.
 */
const uint8_t *proto_get_rcv_buffer(void *msg);

/*
 * Retrieve the sleep interval from the received message
 *
 * Parameters:
 * 	msg : Pointer to received buffer.
 *
 * Returns:
 * 	sleep interval or 0 in case of invalid buffer or msg content
 */
uint32_t proto_get_sleep_interval(void *msg);

/*
 * Retrieve the received data size
 *
 * Parameters:
 * 	msg : Pointer to received buffer.
 *
 * Returns:
 * 	data length in bytes or 0 in case of invalid buffer or msg content
 */
uint32_t proto_get_rcvd_data_len(void *msg);

/*
 * Debug helper function to print the detailed string representation of the
 * protocol message body.
 *
 * Parameters:
 *	msg       : A pointer to an protocol message
 *	tab_level : Tab level at which the message should be printed
 *
 * Returns:
 * 	None
 */
void proto_interpret_msg(void *msg, uint8_t tab_level);


#endif
