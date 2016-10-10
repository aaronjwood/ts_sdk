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
	CC_OK,			/* API exited normally */
	CC_ERROR,		/* API exited with errors */
	CC_INV_PARAM,		/* Invalid parameters passed */
	CC_AUTH_FAILED		/* Authentication failed */
} cc_status;

typedef enum {
	CC_ACK,			/* Received an ACK for the last message sent */
	CC_NACK,		/* Received a NACK for the last message sent */
	CC_SEND_TIMEOUT,	/* Timed out sending the message */
	CC_AUTH_FAILED,		/* Failed to authenticate with the cloud serviec */
} send_status;

/*
 * Initialize the cloud communication API. This will in turn initialize any
 * protocol specific modules, related hardware etc.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	CC_OK    : The module is ready to send / receive data.
 * 	CC_ERROR : An error occurred during initialization.
 */
cc_status cc_init(void);

/*
 * Send bytes to the cloud. The data is wrapped in protocol headers and sent
 * over a secure channel.
 *
 * Parameters:
 * 	sz   : Size of the data in bytes.
 * 	data : Pointer to the data to be sent.
 *
 * Returns:
 * 	CC_OK           : Bytes were sent through the communication channel.
 * 	CC_AUTH_FAILED  : Failed to authenticate with the cloud. The message was
 * 	                  note sent.
 */
cc_status cc_send_bytes_to_cloud(uint16_t sz, uint8_t *data);

#endif
