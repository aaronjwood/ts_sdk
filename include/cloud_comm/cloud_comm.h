/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#ifndef __CLOUD_COMM
#define __CLOUD_COMM

#include <stdint.h>
#include <stdbool.h>
#include "protocol_def.h"

/**
 * \file cloud_comm.h
 *
 * This module forms the device facing API to communicate with the cloud.
 * Sending data to the cloud is a blocking operation. The API accepts raw bytes
 * that need to be transmitted, wraps them in the protocol headers and sends
 * them through selected protocol. A callback is used to report the success or
 * failure of the last transmitted message.
 * The device schedules a receive by specifying a buffer and a callback.
 * The callback is used to report the type of the message received. Scheduling
 * receive is non-blocking.
 */

/**
 * Return values for cc_send_bytes_to_cloud().
 */
typedef enum {
	CC_SEND_FAILED,		/**< Failed to send the message */
	CC_SEND_BUSY,		/**< A message is currently being sent */
	CC_SEND_SUCCESS		/**< Message was sent successfully */
} cc_send_result;

/**
 * Return values for cc_recv_bytes_from_cloud().
 */
typedef enum {
	CC_RECV_FAILED,		/**< Failed to schedule a receive */
	CC_RECV_BUSY,		/**< A receive has already been scheduled */
	CC_RECV_SUCCESS		/**< Successfully scheduled a receive */
} cc_recv_result;

/**
 * Events delivered to the send and receive callback routines.
 */
typedef enum {
	CC_STS_NONE,		/**< Default value; No message body */

	/* Outgoing message events: */
	CC_STS_ACK,		/**< Received an ACK for the last message sent */
	CC_STS_NACK,		/**< Received a NACK for the last message sent */
	CC_STS_SEND_TIMEOUT,	/**< Timed out waiting for a response */

	/* Incoming message events: */
	CC_STS_RCV_CMD_SL,	/**< Received sleep time from the cloud */
	CC_STS_RCV_UPD,		/**< Received an update message from the cloud */
	CC_STS_UNKNOWN		/**< Not supported event */
} cc_event;

typedef uint16_t cc_data_sz;	/**< Type representing the size of the message */

typedef struct {		/* Cloud communication buffer descriptor */
	cc_data_sz bufsz;	/* Maximum size of this buffer */
	void *buf_ptr;		/* Opaque pointer to the actual data buffer */
} cc_buffer_desc;

/*
 * Macro framework for asserting at compile time for the C99 standard; Replace
 * with "static_assert" in C11 and beyond.
 */
#define __token_paste(a, b) a##_##b
#define __get_name(name, line) __token_paste(name, line)
#define __compile_time_assert(predicate, error_text) \
	typedef char __get_name(error_text, __LINE__)[2*!!(predicate) - 1]

#define CC_MIN_RECV_BUF_SZ	PROTO_MIN_RECV_BUF_SZ
#define CC_MIN_SEND_BUF_SZ	PROTO_MIN_SEND_BUF_SZ
#define CC_MAX_SEND_BUF_SZ	PROTO_MAX_SEND_BUF_SZ
#define CC_MAX_RECV_BUF_SZ	PROTO_MAX_RECV_BUF_SZ

/**
 * The buffer defined by CC_RECV_BUFFER is opaque to the user apart from its
 * size.  Headers of this buffer are handled internally. The minimum size of
 * the buffer is CC_MIN_RECV_BUF_SZ and the maximum is CC_MAX_RECV_BUF_SZ.
 */
#define CC_RECV_BUFFER(name, max_sz) \
	__compile_time_assert(((max_sz) <= PROTO_DATA_SZ) && \
			((max_sz) >= (PROTO_CMD_SZ + sizeof(uint32_t))), \
			name##_does_not_have_a_valid_size_on_line); \
	uint8_t name##_bytes[PROTO_OVERHEAD_SZ + (max_sz)]; \
	cc_buffer_desc name = {(max_sz), &(name##_bytes)}

/**
 * The buffer defined by CC_SEND_BUFFER is opaque to the user apart from its
 * size.  Headers of this buffer are handled internally. The minimum size of
 * the buffer is CC_MIN_SEND_BUF_SZ and the maximum is CC_MAX_SEND_BUF_SZ.
 */
#define CC_SEND_BUFFER(name, max_sz) \
	__compile_time_assert(((max_sz) <= PROTO_DATA_SZ) && \
			((max_sz) >= 1), \
			name##_does_not_have_a_valid_size_on_line); \
	uint8_t name##_bytes[(max_sz)]; \
	cc_buffer_desc name = {(max_sz), &(name##_bytes)}

/**
 * Pointer to a callback routine. The callback accepts a buffer, its size and
 * an event from the source of the callback explaining why it was invoked.
 */
typedef void (*cc_callback_rtn)(const void *buf, uint32_t sz, cc_event event);

/**
 * \brief
 * Initialize the cloud communication API.
 *
 * \returns
 *	True  : Initialization was successful.\n
 *	False : Initialization failed.
 *
 * This will in turn initialize any protocol specific modules, related
 * hardware etc. This API must be called before any APIs.
 *
 */
bool cc_init(void);

/**
 * \brief
 * Set the remote host name and host port to communicate with. Port will be used
 * for mostly TCP/IP related protocols and it is optional parameter if low
 * level transport protocol does not support it.
 *
 * \param[in] host : A NULL terminated string specifying the host which includes
 		     tcp/ip host or LTE modem etc...
 * \param[in] port : Optional null terminated string specifying the host port.
 *
 * \returns
 *	True  : Host name and port were set properly.\n
 *	False : Failed to set the host name / host port.
 *
 * This must be called at least once before attempting to send messages.
 */
bool cc_set_destination(const char *host, const char *port);

/**
 * \brief
 * Sets device authorization credentials which will be used to authenticate with
 * the cloud in all future communications. This API is optional if protocol does
 * not need authentication with the cloud othewise required to be called at least
 * once before attempting to commuinicate witht the cloud.
 *
 * \param[in] d_id     : Pointer to a unique device ID.
 * \param[in] d_id_sz  : size of the d_id
 * \param[in] d_sec    : Pointer to the buffer holding the device secret.
 * \param[in] d_sec_sz : Size of the device secret in bytes.
 *
 * \returns
 *	True  : device secrets were set properly.\n
 *	False : Failed to set authentication due to wrong parameters.
 *
 * \note
 * Based on protocol d_id/d_sec can be optional. OTT protocol requires all the
 * fields.
 */
bool cc_set_auth_credentials(const uint8_t *d_id, uint32_t d_id_sz,
 				const uint8_t *d_sec, uint32_t d_sec_sz);
/**
 * \brief
 * Get a pointer to the first byte of the send buffer from the buffer
 * descriptor.
 *
 * \param[in] buf : A cloud communication buffer descriptor.
 *
 * \returns
 * 	Pointer to the send buffer.
 *
 * The data to be sent should be written into this buffer before calling
 * cc_send_bytes_to_cloud().
 *
 */
uint8_t *cc_get_send_buffer_ptr(cc_buffer_desc *buf);

/**
 * \brief
 * Get the value of the sleep interval (in seconds) from the received message.
 *
 * \param[in] buf : A received message.
 *
 * \returns
 * 	Value of the sleep interval in seconds.
 *
 * \note
 * Attempting to retrieve the value from a message that does not contain the
 * sleep interval is unsupported hence it will be only be used when
 * CC_STS_RCV_CMD_SL event is detected.
 *
 */
uint32_t cc_get_sleep_interval(const void *buf);

/**
 * \brief
 * A mandatory API to send bytes to the cloud.
 *
 * \param[in] buf : Pointer to the cloud communication buffer descriptor
 *                  containing the data to be sent.
 * \param[in] sz : Size of the data in bytes.
 * \param[in] cb : Pointer to the callback routine that will be invoked
 *                 with the status of the send.
 *
 * \returns
 * 	CC_SEND_FAILED  : Failed to send the message.\n
 * 	CC_SEND_BUSY    : A send is in progress. \n
 * 	CC_SEND_SUCCESS : Message was sent, waiting for a response from the
 *                        cloud.
 *
 * The data is wrapped in protocol headers and sent over a secure channel.
 * A send is said to be active when it is waiting for a response from the
 * cloud services. Only one send can be active at a time.
 */
cc_send_result cc_send_msg_to_cloud(const cc_buffer_desc *buf, cc_data_sz sz,
		cc_callback_rtn cb);

/**
 * \brief
 * Ask the cloud to send the initial configuration of the device.
 *
 * This function is used when the device has undergone a reset and is no
 * longer in the state the cloud services thinks it is in. A successful call
 * will result in the cloud sending over the initial configuration in a
 * future message.
 *
 * \returns
 * 	CC_SEND_FAILED  : Failed to send the message.\n
 * 	CC_SEND_BUSY    : A send is in progress.\n
 * 	CC_SEND_SUCCESS : Message was sent, waiting for response from the cloud.
 */
cc_send_result cc_resend_init_config(cc_callback_rtn cb);

/**
 * \brief
 * A mandatory API to initiate a receive of bytes from the cloud.
 *
 * \param[in] buf  : Pointer to the cloud communication buffer descriptor
 *                   that will hold the data to be received.
 * \param[in] cb   : Pointer to the callback that will be invoked when a
 *                   complete message is received.
 *
 * \returns
 * 	CC_RECV_FAILED  : Failed to schedule a receive.
 * 	CC_RECV_BUSY    : Can't initiate a receive. One is already in progress.
 * 	CC_RECV_SUCCESS : Successfully initiated a receive.
 *
 * Only one receive can be scheduled at a time. A receive must be scheduled
 * once the API has been initialized.  Scheduling the receive ensures there
 * is a place to receive a response and the appropriate callbacks are invoked.
 */
cc_recv_result cc_recv_bytes_from_cloud(cc_buffer_desc *buf, cc_callback_rtn cb);

/**
 * \brief
 * Call this function periodically to advance the time-based activities.
 *
 * \param[in] cur_ts : The current timestamp representing the system tick
 *                     time in ms.
 *
 * \returns
 * 	Positive value : Number of milliseconds after which this function should
 * 	                 be called again.
 *
 * This function performs time based activities related to managing the
 * the connection to the cloud and the transactions on it. The CC
 * API does not have an internal timer to work with. Instead, this function
 * performs any pending activities and indicates when it next needs to be called
 * to service a time based event.
 */
uint32_t cc_service_send_receive(uint32_t cur_ts);

/**
 * \brief
 * Acknowledge the last message received from the cloud services.
 * \note
 * Actual use is based on the protocol being used, some protocol may never
 * use this API.
 */
void cc_ack_bytes(void);

/**
 * \brief
 * Negatice acknowledgment of the last message received from the cloud services.
 * \note
 * Actual use is based on the protocol being used, some protocol may never
 * use this API.
 */
void cc_nak_bytes(void);

#endif /* __CLOUD_COMM */
