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
 * succeeded or failed.  Callbacks are also used to notify the application
 * when data is received, or other event occurs.  The device must always
 * schedule a receive, specifying a buffer and a callback, in order to receive
 * messsages.  Communication takes place using the protocol (transport)
 * selected by a compile-time option.
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
 * Return values for cc_recv_msg_from_cloud().
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
	CC_EVT_NONE,		/**< Default value; No message body */

	/* Outgoing message events: */
	CC_EVT_SEND_ACKED,	/**< Received an ACK for the last message sent */
	CC_EVT_SEND_NACKED,	/**< Received a NACK for the last message sent */
	CC_EVT_SEND_TIMEOUT,	/**< Timed out waiting for a response */

	/* Incoming message events: */
	CC_EVT_RCVD_MSG,	/**< Received a message from the cloud */
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
	cc_data_sz bufsz;	/* Maximum size of this buffer */
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
	__compile_time_assert(((max_sz) <= CC_MAX_RECV_BUF_SZ) && \
			((max_sz) >= CC_MIN_RECV_BUF_SZ), \
			name##_does_not_have_a_valid_size_on_line); \
	uint8_t name##_bytes[PROTO_OVERHEAD_SZ + (max_sz)]; \
	cc_buffer_desc name = {(max_sz), 0, &(name##_bytes)}

/**
 * The buffer defined by CC_SEND_BUFFER is opaque to the user apart from its
 * size.  Headers of this buffer are handled internally. The minimum size of
 * the buffer is CC_MIN_SEND_BUF_SZ and the maximum is CC_MAX_SEND_BUF_SZ.
 */
#define CC_SEND_BUFFER(name, max_sz) \
	__compile_time_assert(((max_sz) <= CC_MAX_SEND_BUF_SZ) && \
			((max_sz) >= CC_MIN_SEND_BUF_SZ), \
			name##_does_not_have_a_valid_size_on_line); \
	uint8_t name##_bytes[(max_sz)]; \
	cc_buffer_desc name = {(max_sz), 0, &(name##_bytes)}

/**
 * Pointer to a callback routine. The callback receives a buffer descriptor and
 * an event indicating why it was invoked.  The service id indicates which
 * service should process the event.
 */
typedef void (*cc_callback_rtn)(cc_buffer_desc *buf, cc_event event,
				cc_service_id svc_id);

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
 * \param[in] buf : A cloud communication buffer descriptor.
 *
 * \returns
 * 	Pointer to the send buffer.
 *
 * The data to be sent should be written into this buffer before calling
 * cc_send_msg_to_cloud().
 *
 */
uint8_t *cc_get_send_buffer_ptr(cc_buffer_desc *buf);

/**
 * \brief
 * Get a pointer to the first byte of the receive buffer from the buffer
 * descriptor.  Any received data can be read through this buffer.
 *
 * \param[in] buf : A cloud communication buffer descriptor.
 *
 * \returns
 * 	Pointer to the receive buffer.
 */
const uint8_t *cc_get_recv_buffer_ptr(const cc_buffer_desc *buf);

/**
 * \brief
 * Retrieve the length of the last message received.
 *
 * \param[in] buf : A cloud communication buffer descriptor.
 *
 * \returns
 * 	Number of bytes of data present in the receive buffer.
 */
cc_data_sz cc_get_receive_data_len(const cc_buffer_desc *buf);

/**
 * \brief
 * Send a message to the cloud to the specified service.
 *
 * \param[in] buf    : Pointer to the cloud communication buffer descriptor
 *                     containing the data to be sent.
 * \param[in] sz     : Size of the data in bytes.
 * \param[in] svc_id : Id of the service that will process this message.
 * \param[in] cb     : Pointer to the callback routine that will be invoked
 *                     with the status of the send.
 *
 * \returns
 * 	CC_SEND_FAILED  : Failed to send the message.
 * 	CC_SEND_BUSY    : A send is in progress.
 * 	CC_SEND_SUCCESS : Message was sent, waiting for a response from the
 *                        cloud.
 *
 * The data is sent using the selected protocol to the specified service.
 * A send is said to be active when it is waiting for a response from the
 * cloud services. Only one send can be active at a time.
 */
cc_send_result cc_send_svc_msg_to_cloud(const cc_buffer_desc *buf,
					cc_data_sz sz, cc_service_id svc_id,
					cc_callback_rtn cb);
/**
 * \brief
 * Send an application message to the cloud.
 *
 * \param[in] buf    : Pointer to the cloud communication buffer descriptor
 *                     containing the data to be sent.
 * \param[in] sz     : Size of the data in bytes.
 * \param[in] cb     : Pointer to the callback routine that will be invoked
 *                     with the status of the send.
 *
 * \returns
 * 	CC_SEND_FAILED  : Failed to send the message.
 * 	CC_SEND_BUSY    : A send is in progress.
 * 	CC_SEND_SUCCESS : Message was sent, waiting for a response from the
 *                        cloud.
 *
 * The data is sent using the selected protocol to the BASIC service.
 * A send is said to be active when it is waiting for a response from the
 * cloud services. Only one send can be active at a time.
 */
static inline cc_send_result cc_send_msg_to_cloud(const cc_buffer_desc *buf,
						  cc_data_sz sz,
						  cc_callback_rtn cb)
{
	return cc_send_svc_msg_to_cloud(buf, sz, CC_SERVICE_BASIC, cb);
}

/**
 * \brief
 * Initiate receiving a message from the cloud.
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
 * is a place to store an incoming message, or the response to an outgoing
 * message.
 */
cc_recv_result cc_recv_msg_from_cloud(cc_buffer_desc *buf, cc_callback_rtn cb);

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
 * Register a service to process messages that are not basic application data.
 *
 * \param[in] desc : Pointer to a service descriptor that provides the
 *                   service id and entry points for the service.
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
bool cc_register_service(cc_service_descriptor *svc_desc,
			 cc_svc_callback_rtn cb);


/**
 * \brief
 * Dispatch an event to a registered service.  Call this function to allow
 * a service to process a message received on its behalf.
 *
 * \param[in] svc_id : The service id associated with the event.
 *
 * \param[in] buf    : Pointer to the buffer descriptor for the buffer
 *                     containing a message associated with the event.
 *
 * \param[in] event  : The type of the event being dispatched.
 */
void cc_dispatch_event_to_service(cc_service_id svc_id, cc_buffer_desc *buf,
				  cc_event event);

/**
 * \brief
 * Unregister a previously registered service.
 *
 * \param[in] desc : Pointer to a service descriptor that indicates the
 *                   service id and entry points for the service.
 *
 */
void cc_unregister_service(cc_service_descriptor *svc_desc);

#endif /* __CLOUD_COMM */
