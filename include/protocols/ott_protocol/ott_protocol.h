/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#ifndef __OTT_PROTOCOL
#define __OTT_PROTOCOL

#include <stdint.h>
#include <stdbool.h>
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
 * Initialize the OTT Protocol module with device authorization credential which
 * will be needed in all future communications with the cloud serivices.
 * Parameters:
 * 	d_ID     : Pointer to a 16 byte device ID.
 * 	d_sec_sz : Size of the device secret in bytes.
 * 	d_sec    : Pointer to the buffer holding the device secret.
 *
 * Returns:
 * 	PROTO_OK    : Setting authorization credentials was successful.
 * 	PROTO_INV_PARAM : Passed parameters are not valid.
 * Note: This API must be called before ott_send_auth_to_cloud API.
 */
proto_result ott_set_auth(const uint8_t *d_ID, uint16_t d_sec_sz,
                         const uint8_t *d_sec);

/*
 * Retrieves default polling interval in mili seconds
 * Returns:
 * 	Interval time in miliseconds
 */
uint32_t ott_get_default_polling();

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
 * Initialize the OTT Protocol module with device authorization credential which
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
proto_result ott_set_destination(const char *host, const char *port);

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
 * Initiate a secure connection to the cloud service. This involves starting the
 * TCP session and performing the TLS handshake.
 *
 * Parameters:
 * 	host : The server to connect to.
 * 	port : The port to connect to.
 *
 * Returns:
 *	PROTO_OK        : Successfully established a connection.
 *	PROTO_INV_PARAM : NULL pointers were provided in place of host / port.
 *	PROTO_ERROR     : There was an error in establishing the connection.
 *	PROTO_TIMEOUT   : Timed out performing the TLS handshake.
 */
proto_result ott_initiate_connection(const char *host, const char *port);

/*
 * Close the TLS connection
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	PROTO_OK    : Connection closed.
 * 	PROTO_ERROR : Failed to close connection and notify cloud service.
 */
proto_result ott_close_connection(void);

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
proto_result ott_send_bytes_to_cloud(const void *buf, uint32_t sz,
                                        proto_callback cb);

/*
 * Send a control message to the cloud service. This message has no data field
 * associated with it, i.e. the message type is MT_NONE. It is used to poll the
 * cloud for pending messages or to notify it about the termination of the
 * connection or send an ACK / NACK.
 * This message type may or may not have a response.
 * This call is blocking in nature.
 *
 * Parameters:
 * 	c_flags : Control flags
 *
 * Returns:
 * 	PROTO_OK        : Message was sent to the cloud service.
 * 	PROTO_INV_PARAM :	Flag parameter is invalid (Eg. ACK+NACK).
 * 	PROTO_ERROR     : Sending the message failed due to a TCP/TLS error.
 * 	PROTO_TIMEOUT   : Timed out sending the message. Sending failed.
 */
proto_result ott_send_ctrl_msg(c_flags_t c_flags);

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
 * Retrieve the cloud service's most recent response, if any. This call is non-
 * blocking in nature.
 *
 * Parameters:
 * 	msg : Pointer to buffer that will store the message data, type and
 * 	      associated control flags.
 * 	sz  : Maximum size of the buffer.
 *
 * Returns:
 * 	PROTO_OK        : Message successfully retrieved
 * 	PROTO_NO_MSG    : No message to retrieve.
 * 	PROTO_INV_PARAM : A NULL pointer was supplied / invalid maximum buffer size
 * 	PROTO_ERROR     : An error occurred in the TCP/TLS layer.
 */
proto_result ott_retrieve_msg(msg_t *msg, uint16_t sz);

/*
 * Retrieve the binary data from the received message
 *
 * Parameters:
 * 	msg : Pointer to received buffer.
 *
 * Returns:
 * 	data pointer or NULL if fails.
 */
const uint8_t *ott_get_rcv_buffer(void *msg);

/*
 * Retrieve the sleep interval from the received message
 *
 * Parameters:
 * 	msg : Pointer to received buffer.
 *
 * Returns:
 * 	sleep interval or 0 in case of invalid buffer or msg content
 */
uint32_t ott_get_sleep_interval(void *msg);

/*
 * Retrieve the received data size
 *
 * Parameters:
 * 	msg : Pointer to received buffer.
 *
 * Returns:
 * 	data length in bytes or 0 in case of invalid buffer or msg content
 */
uint32_t ott_get_rcvd_data_len(void *msg);

/*
 * Debug helper function to print the string representation of the message type
 * and flag fields of the command byte along with the details about the body of
 * the message.
 *
 * Parameters:
 *	msg       : A pointer to an OTT message
 *	tab_level : Tab level at which the message should be printed
 *
 * Returns:
 * 	None
 */
void ott_interpret_msg(msg_t *msg, uint8_t tab_level);
#endif
