/* Copyright(C) 2017 Verizon. All rights reserved. */

#ifndef __OTT_PROTO
#define __OTT_PROTO

#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"

/*
 * SMSNAS Protocol API:
 * This header lays out the SMSNAS Protocol API which is used to communicate
 * with the cloud. Protocol is asynchronous and either end can send messages
 * at any time.
 */

 /*
  * Initialize the SMSNAS Protocol module. This will initialize the underlying
  * modem drivers. This must be called once before using any of the APIs
  * in this module.
  *
  * Parameters:
  * 	None
  *
  * Returns:
  * 	PROTO_OK    : The API and underlying drivers were properly initialized.
  * 	PROTO_ERROR : Initialization failed.
  */
proto_result smsnas_protocol_init(void);

/*
 * Initialize the SMSNAS Protocol module with the remote host, remote host is
 * the destination cell number
 * Parameters:
 * 	host : A NULL terminated string specifying the host number.
 *
 * Returns:
 * 	PROTO_OK : Setting host and port was successful.
 * 	PROTO_INV_PARAM : Passed parameters are not valid.
 * Note: This must be called at least once before attempting to send messages.
 */
proto_result smsnas_set_destination(const char *host);

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
proto_result smsnas_set_recv_buffer_cb(void *rcv_buf, proto_pl_sz sz,
				           proto_callback rcv_cb);

/*
 * Sends a message to the cloud service. This call is blocking in nature.
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
proto_result smsnas_send_msg_to_cloud(const void *buf, proto_pl_sz sz,
                                        proto_callback cb);

/*
 * Maintenance of the protocol which can be used to complete any
 * pending internal protocol activities before upper level possibly Application
 * decides to sleep for example or relinquish protocol control.
 */
void smsnas_maintenance();

/*
 * Send positive acknowledgment to cloud
 *
 * Parameters:
 *
 *
 * Returns:
 *
 */
void smsnas_send_ack(void);

/*
 * Send negative acknowledgment to cloud.
 *
 * Parameters:
 *
 *
 * Returns:
 *
 */
void smsnas_send_nack(void);

/*
 * Retrieve the binary data pointer from the received message
 *
 * Parameters:
 * 	msg : Starting pointer to received buffer.
 *
 * Returns:
 * 	data pointer or NULL if fails.
 */
const uint8_t *smsnas_get_rcv_buffer_ptr(const void *msg);

/*
 * Retrieve the received data size in bytes
 *
 * Parameters:
 * 	msg : Pointer to received buffer.
 *
 * Returns:
 * 	data length in bytes or 0 in case of invalid buffer or msg content
 */
proto_pl_sz smsnas_get_rcvd_data_len(const void *msg);

/*
 * Maintenance of the protocol which can be used to complete any
 * pending internal protocol activities before upper level possibly Application
 * decides to sleep for example or relinquish protocol control.
 * Returns:
 *      time in miliseconds to call back this function again.
 */
uint32_t smsnas_maintenance(void);

#endif
