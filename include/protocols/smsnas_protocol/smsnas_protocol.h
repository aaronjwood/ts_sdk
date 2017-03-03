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
 *	svs_id: service id which buf belongs to.
 * 	cb : Callback to indicate the event related to send activity.
 *
 * Returns:
 * 	PROTO_OK        : message was sent to the cloud.
 * 	PROTO_INV_PARAM : Invalid parameters
 * 	PROTO_ERROR     : Sending the message failed.
 * 	PROTO_TIMEOUT   : Timed out sending the message. Sending failed.
 */
proto_result smsnas_send_msg_to_cloud(const void *buf, proto_pl_sz sz,
                                proto_service_id svc_id, proto_callback cb);

/*
 * Maintenance of the protocol, Processes pending ack/nack and also calling
 * receive timeout event in case case of concatenated sms did not receive its
 * next segment in a due time
 * Parameters:
 *	poll_due      : True if polling is due to check if concatenated sms's
 *			 next segement has arrived or not.
 *	cur_timestamp : current time stamp in milliseconds to check if
 *		        next segement of the concatenated message is processed.
 */
void smsnas_maintenance(bool poll_due, uint32_t cur_timestamp);

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
 * Retrieves polling interval in mili seconds
 * Returns:
 * 	Interval time in miliseconds
 * Note: Function is mainly used when polling is required to check for arrival of
 *       the next segement in the concatenated message case.
 */
uint32_t smsnas_get_polling_interval(void);

#endif
