/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef __OTT_PROTOCOL
#define __OTT_PROTOCOL

#include <stdint.h>
#include <stdbool.h>

/*
 * OTT Protocol API:
 * This header lays out the OTT Protocol API which is used to communicate with
 * the cloud. The cloud never initiates a connection on its own. Instead, the
 * device polls the cloud at specified intervals to read any messages the cloud
 * wants to send to the device. The device reports its status to the cloud at
 * another set of fixed intervals.
 */

#define VERSION_BYTE		((uint8_t)0x01)
#define UUID_SZ			16

typedef enum {
	/* Generic statuses: */
	OTT_INV_PARAM = 0,	/* Invalid parameter passed to API. */
	OTT_OK,			/* API exited without any errors. */
	OTT_ERROR,		/* API exited with errors. */

	/* Error statuses pertaining to sending messages over TLS: */
	OTT_SEND_NACKED,	/* Received a NACK in response. */
	OTT_SEND_FAILED,	/* Timed out while sending the message. */

	/* Statuses that can combine with the above (via bitwise OR): */
	OTT_MSG_RECVD = 0x80,	/* A message was received. */
} ott_status;

typedef enum {			/* Defines control flags. */
	CF_NONE = 0x00,
	CF_NACK = 0x10,
	CF_ACK = 0x20,
	CF_PENDING = 0x40,
	CF_QUIT = 0x80
} c_flags_t;

typedef enum  {			/* Defines message type flags. */
	MT_NONE = 0,
	MT_AUTH = 1,
	MT_STATUS = 2,
	MT_UPDATE = 3,
	MT_CMD_PI = 10,
	MT_CMD_SL = 11
} m_type_t;

/* Type to store the device ID. */
typedef uint8_t uuid_t[UUID_SZ];

typedef struct {		/* Defines the binary array type. */
	uint16_t size;
	uint8_t *data;
} bytes_t;

typedef union {			/* Possible message values on the receive path. */
	uint32_t cmd_value;
	bytes_t byte_array;
} m_data_t;

typedef struct {		/* A complete message on the receive path. */
	c_flags_t c_flags;
	m_type_t m_type;
	m_data_t data;
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
 * Send the authentication message to the cloud service. Transactions in the TLS
 * session must begin only after a successful call to this API.
 *
 * Parameters:
 *	c_flags : Control flags
 *	dev_id  : The 16 byte device ID.
 *	dev_sec : A binary array that holds the device secret.
 *
 * Returns:
 * 	OTT_OK          : Device was authenticated by the cloud service.
 * 	OTT_SEND_NACKED : Could not authenticate device.
 * 	OTT_SEND_FAILED : Failed to send the authentication message due to timeout.
 * 	OTT_INV_PARAM   : Flag paramter is invalid (Eg. ACK+NACK),
 * 	                  Byte array must have a valid length (<= 508 bytes).
 *
 * 	OTT_MSG_RECVD   : This status might be combined with OTT_OK to notify
 *                        that a message was received from the cloud service.
 */
ott_status ott_send_auth_to_cloud(c_flags_t c_flags,
                                  const uuid_t dev_id,
				  const bytes_t *dev_sec);

/*
 * Send a status message to the cloud service.
 *
 * Parameters:
 *	c_flags : Control flags
 * 	status  : A binary array that stores the data to be reported.
 *
 * Returns:
 * 	OTT_OK          : Status report successfully delivered to the cloud.
 * 	OTT_SEND_NACKED : Cloud did not accept the status report.
 * 	OTT_SEND_FAILED : Failed to send the status report due to timeout.
 * 	OTT_INV_PARAM   : Flag paramter is invalid (Eg. ACK+NACK),
 * 	                  Byte array must have a valid length (<= 508 bytes).
 *
 * 	OTT_MSG_RECVD   : This status might be combined with OTT_OK to notify
 *                        that a message was received from the cloud service.
 */
ott_status ott_send_status_to_cloud(c_flags_t c_flags, const bytes_t *status);

/*
 * Send a control message to the cloud service. This message has no data field
 * associated with it, i.e. the message type is MT_NONE. It is useful if the
 * device does not have anything to send to the cloud but needs to poll the
 * cloud for any possible updates / commands.
 *
 * Paramaters:
 * 	c_flags : Control flags
 *
 * Returns:
 * 	OTT_OK          : Message was successfully sent to the cloud service.
 * 	OTT_SEND_FAILED : Failed to send the message due to timeout.
 * 	OTT_INV_PARAM   : Flag parameter is invalid (Eg. ACK+NACK).
 *
 * 	OTT_MSG_RECVD   : This status might be combined with OTT_OK to notify
 *                        that a message was received from the cloud service.
 */
ott_status ott_send_ctrl_msg(c_flags_t c_flags);

/*
 * Retrieve the message received from the cloud service. This API should be
 * called only if there is a message available to be read, i.e. one of the
 * previous send calls returned (OTT_OK | OTT_MSG_RECVD).
 *
 * Parameters:
 * 	msg : Pointer to buffer that will store the message data, type and
 * 	      associated control flags.
 *
 * Returns:
 * 	OTT_OK    : Message successfully retrieved
 * 	OTT_ERROR : Invalid call. No message to retrieve.
 */
ott_status ott_retrieve_msg(msg_t *msg);
#endif
