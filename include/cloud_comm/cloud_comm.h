/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef __CLOUD_COMM
#define __CLOUD_COMM

#include <stdint.h>
#include <stdbool.h>

/* Cloud communication API:
 * This module forms the device facing API to communicate with the cloud.
 * Sending data to the cloud is a blocking operation. The API accepts raw bytes
 * that need to be transmitted, wraps them in the protocol headers and sends
 * them through a secure channel.
 * Receiving data happens asynchronously. The device schedules a receive by
 * specifying a buffer and a callback. The callback is used to report the
 * success or failure of the last transmitted message.
 * Apart from transmission and reception of data, this API controls the state
 * of the secure channel based on the OTT protocol.
 */

typedef enum {
	CC_SEND_FAILED,
	CC_SEND_BUSY,
	CC_SEND_SUCCESS
} cc_send_result;

typedef enum {
	CC_STS_ACK,		/* Received an ACK for the last message sent */
	CC_STS_NACK,		/* Received a NACK for the last message sent */
	CC_STS_SEND_TIMEOUT,	/* Timed out sending the message */
} cc_status;

/*
 * Initialize the cloud communication API. This will in turn initialize any
 * protocol specific modules, related hardware etc. It also sets the device ID
 * and device secret. These two are used to authenticate the device with the
 * cloud service and are unlikely to change during the lifetime of the device.
 *
 * Parameters:
 * 	d_ID     : Pointer to a 16 byte device ID.
 * 	d_sec_sz : Size of the device secret in bytes.
 * 	d_sec    : Pointer to the buffer holding the device secret.
 *
 * Returns:
 *	True  : Initialization was successful.
 *	False : Initialization failed.
 */
bool cc_init(const uint8_t *d_ID, uint16_t d_sec_sz, const uint8_t *d_sec);

/*
 * Register a receive callback.
 *
 * Parameters:
 * 	cb : Pointer to a function that will serve as the receive callback.
 *
 * Returns:
 * 	None
 */
typedef void (*cc_rx_cb)(cc_status status);
void cc_set_rx_callback(cc_rx_cb cb);

/*
 * Send bytes to the cloud. The data is wrapped in protocol headers and sent
 * over a secure channel.
 *
 * Parameters:
 * 	sz   : Size of the data in bytes.
 * 	data : Pointer to the data to be sent.
 *
 * Returns:
 * 	CC_SEND_FAILED  : Failed to send the message.
 * 	CC_SEND_BUSY    : A send is in progress.
 * 	CC_SEND_SUCCESS : Message was sent, waiting for a response from the cloud.
 */
cc_send_result cc_send_bytes_to_cloud(uint16_t sz, uint8_t *data);

#endif
