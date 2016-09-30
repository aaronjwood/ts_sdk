/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef __OTT_PROTOCOL
#define __OTT_PROTOCOL

#include <stdint.h>
#include <stdbool.h>

/*
 * OTT Protocol API:
 * This header lays out the OTT Protocol API which is used to communicate with
 * the cloud. The cloud never initiates a connection on its own. Instead, the
 * device polls the cloud at specified intervals and also whenever it sends data,
 * to read any messages back from the cloud.
 * There is never more than one outstanding message in this protocol.
 * The device reports its status to the cloud at another set of fixed intervals.
 */

#define VERSION_BYTE		((uint8_t)0x01)
#define UUID_SZ			16
#define MAX_MSG_SZ		512
#define VER_SZ			1
#define CMD_SZ			1
#define LEN_SZ			2
#define MAX_DATA_SZ		(MAX_MSG_SZ - VER_SZ - CMD_SZ - LEN_SZ)

typedef enum {
	OTT_OK,			/* API exited without any errors. */
	OTT_ERROR,		/* API exited with errors. */
	OTT_SEND_FAILED,	/* Timed out waiting for a response from the
				 * cloud service. */
} ott_status;

typedef enum {			/* Defines control flags. */
	CF_NONE = 0x00,		/* No flag set */
	CF_NACK = 0x10,		/* Failed to accept or process previous message */
	CF_ACK = 0x20,		/* Previous message accepted */
	CF_PENDING = 0x40,	/* More messages to follow */
	CF_QUIT = 0x80		/* Close connection. All messages sent / received. */
} c_flags_t;

typedef enum  {			/* Defines message type flags. */
	MT_NONE = 0,		/* Control message */
	MT_AUTH = 1,		/* Authentication message sent by device */
	MT_STATUS = 2,		/* Status report sent by device */
	MT_UPDATE = 3,		/* Update message received by device */
	MT_CMD_PI = 10,		/* Cloud instructs to set new polling interval */
	MT_CMD_SL = 11		/* Cloud instructs device to sleep */
} m_type_t;

/* Type to store the device ID (stored in Little Endian format). */
typedef uint8_t uuid_t[UUID_SZ];

/* A complete message on the receive path. */
typedef struct __attribute__((packed)) {
	c_flags_t c_flags;
	m_type_t m_type;
	union {
		uint32_t cmd_value;
		struct __attribute__((packed)) {
			uint16_t sz;
			uint8_t bytes[MAX_DATA_SZ];
		} array;
	};
} msg_t;

/*
 * Initialize the OTT Protocol module. This will initialize the underlying modem
 * / TCP drivers, set up the TLS parameters and initialize any certificates, if
 * needed.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	OTT_OK    : The API and underlying drivers were properly initialized.
 * 	OTT_ERROR : Initialization failed.
 */
ott_status ott_protocol_init(void);

/*
 * Initiate a connection to the cloud service. This involves starting the TCP
 * session, TLS handshake and verification of server certificates.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 *	OTT_OK     : Successfully established a connection.
 *	OTT_ERROR  : There was an error in establishing the connection.
 */
ott_status ott_initiate_connection(void);

/*
 * Close the TLS connection and notify the cloud service.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	OTT_OK    : Connection closed.
 * 	OTT_ERROR : Failed to close connection and notify cloud service.
 */
ott_status ott_close_connection(void);

/*
 * Send APIs:
 * These APIs are of three kinds:
 * 	1) Send authentication message
 * 	2) Send status report
 * 	3) Send a control message
 * On calling 1) and 2), the cloud produces a response that must be read before
 * taking the next course of action. This is done through the
 * ott_retrieve_msg(...) API.
 * The third send API may or may not produce a response from the cloud depending
 * on what flags are set.
 */

/*
 * Send the authentication message to the cloud service. Transactions in the TLS
 * session must begin only after a successful call to this API (i.e. it exits
 * with OTT_OK) and after checking if the ACK flag is set in the received response.
 *
 * Parameters:
 *	c_flags    : Control flags
 *	dev_id     : The 16 byte device ID.
 *	dev_sec_sz : Size of the device secret in bytes.
 *	dev_sec    : Pointer to binary data that holds the device secret.
 *
 * Returns:
 * 	OTT_OK          : Authentication message was sent to the cloud and a
 * 	                  response was received in return.
 * 	OTT_SEND_FAILED : Timed out waiting for a response from the cloud service.
 * 	                  The authentication message was not delivered.
 * 	OTT_ERROR       : Possible causes of error:
 * 	                  Flag parameter is invalid (Eg. ACK+NACK),
 * 	                  Byte array must have a valid length (<= MAX_DATA_SZ bytes),
 * 	                  Sending the message failed due to a TCP/TLS error.
 */
ott_status ott_send_auth_to_cloud(c_flags_t c_flags, const uuid_t dev_id,
				  uint16_t dev_sec_sz, const uint8_t *dev_sec);

/*
 * Send a status message to the cloud service. The message is successfully
 * delivered if this call exits with OTT_OK and the received response has the
 * ACK flag set.
 *
 * Parameters:
 *	c_flags   : Control flags
 *	status_sz : Size of the status data in bytes.
 * 	status    : Pointer to binary data that stores the status to be reported.
 *
 * Returns:
 * 	OTT_OK          : Status report was sent to the cloud and a response was
 * 	                  received in return.
 * 	OTT_SEND_FAILED : Timed out waiting for a response from the cloud service.
 * 	                  The status report was not delivered.
 * 	OTT_ERROR       : Possible causes of error:
 * 	                  Flag parameter is invalid (Eg. ACK+NACK),
 * 	                  Byte array must have a valid length (<= MAX_DATA_SZ bytes),
 * 	                  Sending the message failed due to a TCP/TLS error.
 */
ott_status ott_send_status_to_cloud(c_flags_t c_flags,
                                    uint16_t status_sz,
				    const uint8_t *status);

/*
 * Send a control message to the cloud service. This message has no data field
 * associated with it, i.e. the message type is MT_NONE.
 * It is useful when the device does not have any data to report but needs to
 * poll the cloud for pending updates and commands, ACK or NACK a previous
 * message response from the cloud, or needs to end the connection.
 * The pending flag cannot be active for this type of message.
 * This message type may or may not have a response.
 *
 * Paramaters:
 * 	c_flags : Control flags
 *
 * Returns:
 * 	OTT_OK    : Message was sent to the cloud service.
 * 	OTT_ERROR : Possible causes of error:
 * 	            Flag parameter is invalid (Eg. ACK+NACK).
 * 	            Sending the message failed due to a TCP/TLS error.
 */
ott_status ott_send_ctrl_msg(c_flags_t c_flags);

/*
 * The device has to poll for responses from the cloud. It does so right after
 * sending a message or via a control message in case it has nothing to send
 * while the cloud has pending messages. Use this function to retrieve the cloud
 * service's most recent response.
 *
 * Parameters:
 * 	msg : Pointer to buffer that will store the message data, type and
 * 	      associated control flags.
 *
 * Returns:
 * 	OTT_OK    : Message successfully retrieved
 * 	OTT_ERROR : No message to retrieve.
 */
ott_status ott_retrieve_msg(msg_t *msg);
#endif
