/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#ifndef __CLOUD_COMM
#define __CLOUD_COMM

#include <stdint.h>
#include <stdbool.h>
#include "service_ids.h"
#include "protocol_def.h"

/**
 * \file cloud_comm.h
 *
 * This module defines the API used by devices to communicate with the cloud.
 * Data is sent and received as discrete messages passed over the selected
 * protocol.  A callback is used to notify the application if a send
 * succeeded or failed, when data is received, or when other events occur.
 * Communication takes place using the protocol (transport) selected by a
 * compile-time option.
 */

/**
 * Return values for cc_send_msg_to_cloud().
 */
typedef enum {
	CC_SEND_FAILED,		/**< Failed to send the message */
	CC_SEND_BUSY,		/**< A message is currently being sent */
	CC_SEND_SUCCESS		/**< Message was sent successfully */
} cc_send_result;

/**
 * Return values for cc_set_recv_buffer()
 */
typedef enum {
	CC_RECV_FAILED,		/**< Failed to set the receive buffer. */
	CC_RECV_BUSY,		/**< Cannot change buffer since it is in use. */
	CC_RECV_SUCCESS		/**< Successfully set a receive buffer */
} cc_set_recv_result;

/**
 * Events delivered to the send and receive callback routines.
 */
typedef enum {
	CC_EVT_NONE,		/**< Default value; No message body */

	/* Outgoing message events: */
	CC_EVT_SEND_ACKED,	/**< Received an ACK for the last message sent */
	CC_EVT_SEND_NACKED,	/**< Received a NACK for the last message sent */
	CC_EVT_SEND_TIMEOUT,	/**< Timed out waiting for a response */

	/* Incoming message events: */
	CC_EVT_RCVD_MSG,	/**< Received a message from the cloud */
	/**< Received a bad message from the cloud indicating out of protocol
	 spec messages */
	CC_EVT_RCVD_WRONG_MSG,
	CC_EVT_RCVD_MSG_HIGHPRIO, /**< Received high priority message. */
	CC_EVT_RCVD_OVERFLOW,   /**< Received message too big for buffer. */

	/* Control message events delivered to a Control callback */
	CC_EVT_CTRL_SLEEP,	/**< Refrain from sending (i.e. airplane mode)*/
	CC_EVT_CTRL_FACTORY_RST,/**< Protocol asked device to do factory reset*/

	/* Other events are defined by their respective services */
	CC_EVT_SERVICES_BASE=1024,

} cc_event;

typedef uint16_t cc_data_sz;	/**< Type representing the size of a message */

typedef uint8_t cc_service_id;  /**< Type representing a service-id */

typedef struct {		/* Cloud communication buffer descriptor */
	cc_data_sz bufsz;	/* Size of this buffer not counting space
				   reserved for protocol overhead. */
	cc_data_sz current_len; /* Total length of rcvd data in buffer,
				   including protocol overhead */
	void *buf_ptr;		/* Opaque pointer to the actual data buffer */
} cc_buffer_desc;

typedef struct cc_service_descriptor cc_service_descriptor;

/*
 * Macro framework for asserting at compile time for the C99 standard; Replace
 * with "static_assert" in C11 and beyond.
 */
#define __token_paste(a, b) a##_##b
#define __get_name(name, line) __token_paste(name, line)
#define __compile_time_assert(predicate, error_text) \
	typedef char __get_name(error_text, __LINE__)[2*!!(predicate) - 1]

/**
 * The size of buffer needed to send and receive any possible message
 * using the current protocol (not counting protocol overhead).
 * Use one of these values with the CC_SEND_BUFFER/CC_RECV_BUFFER macros
 * to allocate space for the largest possible message used by any service.
 */
#define CC_MAX_SEND_BUF_SZ	PROTO_MAX_SEND_BUF_SZ
#define CC_MAX_RECV_BUF_SZ	PROTO_MAX_RECV_BUF_SZ

/**
 * The buffer defined by CC_RECV_BUFFER is opaque to the user apart from its
 * size.  Space for protocol overhead is automatically added.
 * The maximum size for the buffer is CC_MAX_RECV_BUF_SZ.
 */
#define CC_RECV_BUFFER(name, data_sz) \
	__compile_time_assert(((data_sz) <= CC_MAX_RECV_BUF_SZ), \
			name##_does_not_have_a_valid_size_on_line); \
	uint8_t name##_bytes[PROTO_OVERHEAD_SZ + (data_sz)]; \
	cc_buffer_desc name = {(data_sz), 0, &(name##_bytes)}

/**
 * The buffer defined by CC_SEND_BUFFER is opaque to the user apart from its
 * size.  Space for protocol overhead is automatically added.
 * The maximum size for the buffer is CC_MAX_SEND_BUF_SZ.
 */
#define CC_SEND_BUFFER(name, data_sz) \
		__compile_time_assert(((data_sz) <= CC_MAX_SEND_BUF_SZ), \
				name##_does_not_have_a_valid_size_on_line); \
	uint8_t name##_bytes[(data_sz)]; \
	cc_buffer_desc name = {(data_sz), 0, &(name##_bytes)}

/**
 * Pointer to a service callback routine.  The callback receives an event
 * indicating why it was invoked, and optional event parameter values,
 * as defined for each event.  A service callback is called when a registered
 * service requires the application to take some action in response to a
 * message.
 *
 * Every application must provide a service callback for the Control service,
 * as part of initializing the cloud communication API.
 */
typedef void (*cc_svc_callback_rtn)(cc_event event, uint32_t value, void *ptr);

/**
 * \brief
 * Initialize the cloud communication API.
 *
 * \param[in] cb : Pointer to the callback routine that will be invoked
 *                 when a Control event requires application action.
 *
 * \returns
 *	True  : Initialization was successful.
 *	False : Initialization failed.
 *
 * This will in turn initialize any protocol specific modules, related
 * hardware etc. This function must be called before any other cloud_comm
 * functiion.
 *
 */
bool cc_init(cc_svc_callback_rtn control_cb);

/**
 * \brief
 * Sets the destination for cloud communications.
 * For a TCP/IP based protocol (i.e. OTT), specify the hostname and port
 * number in the form:
 *     hostname:port.
 * This function does not need to be called if the protocol does not
 * require a destination.
 *
 * \param[in] dest : A null terminated string specifying the destination.
 *
 * \returns
 *	True  : Destination was set successfully.
 *	False : Failed to set the destination.
 *
 * If required by the protocol, this must be called at least once before
 * attempting to send messages.
 */
bool cc_set_destination(const char *dest);

/**
 * \brief
 * Sets device authorization credentials which will be used to authenticate with
 * the cloud in all future communications.  If required by the protocol,
 * this must be called at least once before attempting to communicate with the
 * cloud.
 *
 * \param[in] d_id     : Pointer to a unique device ID.
 * \param[in] d_id_sz  : size of the d_id
 * \param[in] d_sec    : Pointer to the buffer holding the device secret.
 * \param[in] d_sec_sz : Size of the device secret in bytes.
 *
 * \returns
 *	True  : device secrets were set properly.
 *	False : Failed to set authentication due to wrong parameters.
 *
 * \note
 * This function need not be called if the protocol is implicitly authenticated
 * to the cloud e.g. by its cellular network identity.
 * For the OTT protocol, both the id and secret must be supplied.
 */
bool cc_set_auth_credentials(const uint8_t *d_id, uint32_t d_id_sz,
 				const uint8_t *d_sec, uint32_t d_sec_sz);

/**
 * \brief
 * Get a pointer to the first byte of the send buffer from the buffer
 * descriptor.
 *
 * \param[in] buf    : A cloud communication buffer descriptor.
 * \param[in] svc_id : Service id of the message in the buffer.
 *
 * \returns
 * 	Pointer to the send buffer.
 *
 * The data to be sent should be written into this buffer before calling
 * cc_send_msg_to_cloud().
 *
 */
uint8_t *cc_get_send_buffer_ptr(cc_buffer_desc *buf, cc_service_id svc_id);

/**
 * \brief
 * Get a pointer to the first byte of the receive buffer from the buffer
 * descriptor.  Any received data can be read through this buffer.
 *
 * \param[in] buf : A cloud communication buffer descriptor.
 * \param[in] svc_id : Service id of the message in the buffer.
 *
 * \returns
 * 	Pointer to the receive buffer.
 */
const uint8_t *cc_get_recv_buffer_ptr(const cc_buffer_desc *buf,
				      cc_service_id svc_id);

/**
 * \brief
 * Retrieve the length of the last message received.
 *
 * \param[in] buf : A cloud communication buffer descriptor.
 * \param[in] svc_id : Service id of the message in the buffer.
 *
 * \returns
 * 	Number of bytes of data present in the receive buffer.
 */
cc_data_sz cc_get_receive_data_len(const cc_buffer_desc *buf,
				   cc_service_id svc_id);

/**
 * \brief
 * Send a message to the cloud to the specified service.
 *
 * \param[in] buf    : Pointer to the cloud communication buffer descriptor
 *                     containing the data to be sent.
 * \param[in] sz     : Size of the data in bytes.
 * \param[in] svc_id : Id of the service that will process this message.
 *
 * \returns
 * 	CC_SEND_FAILED  : Failed to send the message.
 * 	CC_SEND_BUSY    : A send is in progress.
 * 	CC_SEND_SUCCESS : Message was sent, waiting for a response from the
 *                        cloud.
 *
 * The data is sent using the selected protocol to the specified service.
 *
 * \note
 * For MQTT protocol this sends message as a part of command response and svc_id
 * field will be ignored. For other protocols, it is svc_id service message.
 */
cc_send_result cc_send_svc_msg_to_cloud(cc_buffer_desc *buf,
					cc_data_sz sz, cc_service_id svc_id);

/**
 * \brief
 * Send a status message to the cloud.
 *
 * \param[in] buf    : Pointer to the cloud communication buffer descriptor
 *                     containing the data to be sent.
 * \param[in] sz     : Size of the data in bytes.
 * \param[in] svc_id : Id of the service that will process this message.
 *
 * \returns
 * 	CC_SEND_FAILED  : Failed to send the message.
 * 	CC_SEND_BUSY    : A send is in progress.
 * 	CC_SEND_SUCCESS : Message was sent, waiting for a response from the
 *                        cloud.
 *
 * The data is sent using the selected protocol to the specified service.
 *
 * \note
 * For MQTT protocol this sends message to onboard topic, mqtt message generally
 * contains device info, app data etc...For other protocols, it is
 * basic service message.
 */
cc_send_result cc_send_status_msg_to_cloud(cc_buffer_desc *buf, cc_data_sz sz);

/**
 * \brief
 * Make a buffer available to hold received messages.
 *
 * \param[in] buf  : Pointer to the cloud communication buffer descriptor
 *                   that will hold the data to be received.
 *
 * \returns
 * 	CC_RECV_FAILED  : Failed to set the buffer.
 * 	CC_RECV_BUSY    : Cannot change buffer since it is in use.
 * 	CC_RECV_SUCCESS : Buffer was successfully set.
 *
 * Only one buffer can be set at a time. A receive buffer must be made available
 * once the API has been initialized.  This ensures there is a place to
 * store any incoming messages, or the response to an outgoing
 * message.
 *
 * In normal usage, the applicaton should define and make available a
 * CC_RECV_BUFFER() of CC_MAX_RECV_BUF_SZ bytes.
 */
cc_set_recv_result cc_set_recv_buffer(cc_buffer_desc *buf);

/**
 * \brief
 * Call this function periodically to advance the time-based activities.
 *
 * \param[in] cur_ts : The current timestamp representing the system tick
 *                     time in ms.
 *
 * \returns
 * 	Positive value : Number of milliseconds after which this function should
 * 	                 be called again. 0 is a special value where upper level
 *			 can sleep indefinitely without needing to call this
 *			 function until low level hardware activity wakes it up
 *
 * This function performs time based activities related to managing the
 * the connection to the cloud and the transactions on it. The CC
 * API does not have an internal timer to work with. Instead, this function
 * performs any pending activities and indicates when it next needs to be called
 * to service a time based event.
 */
uint32_t cc_service_send_receive(uint64_t cur_ts);

/**
 * \brief
 * Acknowledge the last message received from the cloud services.
 * \note
 * The exact behavior is protocol-dependent, but applications must always call
 * this function or cc_nack_msg
 */
void cc_ack_msg(void);

/**
 * \brief
 * Send a negative acknowledgment of the last message received from the cloud
 * services.
 * \note
 * The exact behavior is protocol-dependent, but applications must always call
 * this function or cc_ack_msg
 */
void cc_nak_msg(void);

/*
 * Functions to support additional services.
 */

/**
 * \brief
 * Register to process events and data for the specified service.
 *
 * \param[in] desc : Pointer to a service descriptor that provides the
 *                   service id and other registration data.
 *
 * \param[in] cb   : Pointer to the callback routine that will be invoked
 *                   when the service generates an event which requires
 *		     application action.
 *
 * \returns
 *	True  : The service was registered.
 *	False : Registration failed.
 *
 * \note
 * The Control service is registered automatically.
 */
bool cc_register_service(const cc_service_descriptor *svc_desc,
			 cc_svc_callback_rtn cb);

#endif /* __CLOUD_COMM */
