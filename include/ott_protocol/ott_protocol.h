/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef __OTT_PROTOCOL
#define __OTT_PROTOCOL

#include <stdint.h>
#include <stdbool.h>
#include "ott_limits.h"

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
 * Define this to profile the OTT functions. The time recorded also includes the
 * time taken to receive the data over the network and time spent in the modem.
 * The initialization function is not profiled.
 */
/*#define OTT_TIME_PROFILE*/

/*
 * Define this to explicitly show the time it takes to connect to the network /
 * send / receive data over the network. This allows to compute how much time is
 * spent in pure crypto computation / preparing and unwrapping the data packets.
 * OTT_TIME_PROFILE must be defined for this to work.
 */
/*#define OTT_EXPLICIT_NETWORK_TIME*/

/*
 * Define this to profile the maximum heap used by mbedTLS. This is done by
 * providing a custom implementation of calloc(). Everytime the connection is
 * closed through ott_close_connection(), the maximum amount of heap used until
 * that point of time is printed to the debug UART.
 */
/*#define OTT_HEAP_PROFILE*/

#ifdef OTT_EXPLICIT_NETWORK_TIME
extern uint32_t network_time_ms;
#endif

typedef enum {			/* Defines return codes of this API. */
	OTT_OK,			/* API call exited without any errors */
	OTT_ERROR,		/* API call exited with errors */
	OTT_TIMEOUT,		/* Timed out waiting for API to finish */
	OTT_NO_MSG,		/* There are no messages to be retrieved */
	OTT_INV_PARAM		/* Invalid parameters passed to the API */
} ott_status;

typedef enum {			/* Defines control flags. */
	CF_NONE = 0x00,		/* No flag set */
	CF_NACK = 0x10,		/* Failed to accept or process previous message */
	CF_ACK = 0x20,		/* Previous message accepted */
	CF_PENDING = 0x40,	/* More messages to follow */
	CF_QUIT = 0x80		/* Close connection */
} c_flags_t;

typedef enum  {			/* Defines message type flags. */
	MT_NONE = 0,		/* Control message */
	MT_AUTH = 1,		/* Authentication message sent by device */
	MT_STATUS = 2,		/* Status report sent by device */
	MT_UPDATE = 3,		/* Update message received by device */
	MT_RESTARTED = 4,	/* Lets the cloud know the device restarted */
	MT_CMD_PI = 10,		/* Cloud instructs to set new polling interval */
	MT_CMD_SL = 11		/* Cloud instructs device to sleep */
} m_type_t;

/* Helper macros to query if certain flag bits are set in the command byte. */
#define OTT_FLAG_IS_SET(var, flag)	\
	(((flag) == CF_NONE) ? ((var) == CF_NONE) : ((var) & (flag)) == (flag))

/* Helper macros to interpret the command byte. */
#define OTT_LOAD_FLAGS(cmd, f_var)	((f_var) = (uint8_t)(cmd) & 0xF0)
#define OTT_LOAD_MTYPE(cmd, m_var)	((m_var) = (uint8_t)(cmd) & 0x0F)

/* Defines an array type */
typedef struct __attribute__((packed)) {
	uint16_t sz;			/* Number of bytes currently filled */
	uint8_t bytes[];
} array_t;

/* Defines a value received by the device from the cloud */
typedef union __attribute__((packed)) {
	uint32_t interval;
	array_t array;
} msg_packet_t;

/* A complete OTT protocol message */
typedef struct __attribute__((packed)) {
	uint8_t cmd_byte;
	msg_packet_t data;
} msg_t;

/*
 * Initialize the OTT Protocol module. This will initialize the underlying modem
 * / TCP drivers, set up the TLS parameters and initialize any certificates, if
 * needed. This must be called once before using any of the APIs in this module.
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
 * Initiate a secure connection to the cloud service. This involves starting the
 * TCP session and performing the TLS handshake.
 *
 * Parameters:
 * 	host : The server to connect to.
 * 	port : The port to connect to.
 *
 * Returns:
 *	OTT_OK        : Successfully established a connection.
 *	OTT_INV_PARAM : NULL pointers were provided in place of host / port.
 *	OTT_ERROR     : There was an error in establishing the connection.
 *	OTT_TIMEOUT   : Timed out performing the TLS handshake.
 */
ott_status ott_initiate_connection(const char *host, const char *port);

/*
 * Close the TLS connection and notify the cloud service. This function must
 * be called before ott_initiate_connection() except for the very first time.
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
 * session must begin only after a successful call to this API (i.e. it exits
 * with OTT_OK) and after checking if the ACK flag is set in the received response.
 * An auth message can be sent only after a call to ott_initiate_connection().
 * This call is blocking in nature.
 *
 * Parameters:
 *	c_flags    : Control flags
 *	dev_id     : Pointer to the 16 byte device ID.
 *	dev_sec_sz : Size of the device secret in bytes.
 *	dev_sec    : Pointer to binary data that holds the device secret.
 *
 * Returns:
 * 	OTT_OK        : Authentication message was sent to the cloud.
 * 	OTT_INV_PARAM :	Flag parameter is invalid (Eg. ACK+NACK) or
 * 	                dev_sec has length > MAX_DATA_SZ bytes or
 * 	                Device secret / device ID is NULL or dev_sec_sz is zero.
 * 	OTT_ERROR     : Sending the message failed due to a TCP/TLS error.
 * 	OTT_TIMEOUT   : Timed out sending the message. Sending failed.
 */
ott_status ott_send_auth_to_cloud(c_flags_t c_flags, const uint8_t *dev_id,
				  uint16_t dev_sec_sz, const uint8_t *dev_sec);

/*
 * Send a status message to the cloud service. The message is successfully
 * delivered if this call exits with OTT_OK and the received response has the
 * ACK flag set.
 * This call is blocking in nature.
 *
 * Parameters:
 *	c_flags   : Control flags
 *	status_sz : Size of the status data in bytes.
 * 	status    : Pointer to binary data that stores the status to be reported.
 *
 * Returns:
 * 	OTT_OK        : Status report was sent to the cloud.
 * 	OTT_INV_PARAM :	Flag parameter is invalid (Eg. ACK+NACK) or
 * 	                status has length > MAX_DATA_SZ bytes or
 * 	                status is NULL
 * 	OTT_ERROR     : Sending the message failed due to a TCP/TLS error.
 * 	OTT_TIMEOUT   : Timed out sending the message. Sending failed.
 */
ott_status ott_send_status_to_cloud(c_flags_t c_flags,
                                    uint16_t status_sz,
				    const uint8_t *status);

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
 * 	OTT_OK        : Message was sent to the cloud service.
 * 	OTT_INV_PARAM :	Flag parameter is invalid (Eg. ACK+NACK).
 * 	OTT_ERROR     : Sending the message failed due to a TCP/TLS error.
 * 	OTT_TIMEOUT   : Timed out sending the message. Sending failed.
 */
ott_status ott_send_ctrl_msg(c_flags_t c_flags);

/*
 * Notify the cloud service that the device has restarted and needs to retrieve
 * its initial data. A restart could be a result of a number of things, such as
 * a watch dog timeout, power glitch etc.
 * This message type does have a response.
 * This call is blocking in nature.
 *
 * Parameters:
 * 	c_flags : Flags to send with this message.
 *
 * Returns:
 * 	OTT_OK      : Message was sent to the cloud service.
 * 	OTT_ERROR   : Sending the message failed due to a TCP/TLS error.
 * 	OTT_TIMEOUT : Timed out sending the message. Sending failed.
 */
ott_status ott_send_restarted(c_flags_t c_flags);

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
 * 	OTT_OK        : Message successfully retrieved
 * 	OTT_NO_MSG    : No message to retrieve.
 * 	OTT_INV_PARAM : A NULL pointer was supplied / invalid maximum buffer size
 * 	OTT_ERROR     : An error occurred in the TCP/TLS layer.
 */
ott_status ott_retrieve_msg(msg_t *msg, uint16_t sz);

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
