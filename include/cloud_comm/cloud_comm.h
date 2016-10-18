/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef __CLOUD_COMM
#define __CLOUD_COMM

#include <stdint.h>
#include <stdbool.h>
#include "ott_limits.h"

/* Cloud communication API:
 * This module forms the device facing API to communicate with the cloud.
 * Sending data to the cloud is a blocking operation. The API accepts raw bytes
 * that need to be transmitted, wraps them in the protocol headers and sends
 * them through a secure channel. A callback is used to report the success or
 * failure of the last transmitted message.
 * Receiving data happens asynchronously. The device schedules a receive by
 * specifying a buffer and a callback. The callback is used to report the
 * type of the message received.
 * There are three kinds of messages on the receive path:
 * 	Update   : Application specific byte arrays
 * 	Commands : Used to set reporting and polling interval values
 * 	Control  : Messages from the cloud with a command byte and an empty body
 * Apart from transmission and reception of data, this API controls the state
 * of the secure channel based on the OTT protocol.
 */

typedef enum {
	CC_SEND_FAILED,
	CC_SEND_BUSY,
	CC_SEND_SUCCESS
} cc_send_result;

typedef enum {
	CC_RECV_BUSY,
	CC_RECV_SUCCESS
} cc_recv_result;

typedef enum {
	/* Outgoing path events: */
	CC_STS_ACK,		/* Received an ACK for the last message sent */
	CC_STS_NACK,		/* Received a NACK for the last message sent */
	CC_STS_SEND_TIMEOUT,	/* Timed out waiting for a response */

	/* Incoming path events: */
	CC_STS_RCV_CTRL,	/* Received a control message from the cloud */
	CC_STS_RCV_CMD_PI,	/* Received polling interval from the cloud */
	CC_STS_RCV_CMD_SL,	/* Received sleep time from the cloud */
	CC_STS_RCV_UPD		/* Received an update message from the cloud */
} cc_event;

typedef uint16_t cc_data_sz;

typedef struct {		/* Cloud communication buffer descriptor */
	cc_data_sz bufsz;
	void *buf_ptr;
} cc_buffer_desc;

/*
 * Macro framework for asserting at compile time for the C99 standard; Replace
 * with "static_assert" in C11 and beyond.
 */
#define __token_paste(a, b) a##_##b
#define __get_name(name, line) __token_paste(name, line)
#define __compile_time_assert(predicate, error_text) \
	typedef char __get_name(error_text, __LINE__)[2*!!(predicate) - 1]

/*
 * The buffer defined by CC_*_BUFFER is opaque to the user apart from its size.
 * Headers of this buffer are handled internally. The minimum size of the buffer
 * is 4 bytes. The maximum buffer size is OTT_DATA_SZ.
 */
#define CC_RECV_BUFFER(name, max_sz) \
	__compile_time_assert(((max_sz) <= OTT_DATA_SZ) && \
			((max_sz) >= sizeof(uint32_t)), \
			name##_does_not_have_a_valid_size_on_line); \
	uint8_t name##_bytes[OTT_OVERHEAD_SZ + (max_sz)]; \
	cc_buffer_desc name = {(max_sz), &(name##_bytes)}

#define CC_SEND_BUFFER(name, max_sz) \
	__compile_time_assert(((max_sz) <= OTT_DATA_SZ) && \
			((max_sz) >= sizeof(uint32_t)), \
			name##_does_not_have_a_valid_size_on_line); \
	uint8_t name##_bytes[(max_sz)]; \
	cc_buffer_desc name = {(max_sz), &(name##_bytes)}

/*
 * Pointer to callback routine. The callback accepts a buffer descriptor and
 * an event from the source of the callback explaining why it was invoked.
 */
typedef void (*cc_callback_rtn)(cc_buffer_desc *buf, cc_event status);

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
 * Get a pointer to the internal send buffer from the buffer descriptor. The
 * data to be sent should be written onto this buffer before calling
 * cc_send_bytes_to_cloud().
 *
 * Parameters:
 * 	buf : A cloud communication buffer descriptor.
 *
 * Returns:
 * 	Pointer to the internal send buffer.
 */
uint8_t *cc_get_send_buffer_ptr(cc_buffer_desc *buf);

/*
 * Get a pointer to the internal receive buffer from the buffer descriptor. Any
 * received data can be read through this buffer.
 *
 * Parameters:
 * 	buf : A cloud communication buffer descriptor.
 *
 * Returns:
 * 	Pointer to the internal receive buffer.
 */
uint8_t *cc_get_recv_buffer_ptr(cc_buffer_desc *buf);

/*
 * Retrieve the length of the last message received.
 *
 * Parameters:
 * 	buf : A cloud communication buffer descriptor.
 *
 * Returns:
 * 	Number of bytes of data in the receive buffer.
 */
cc_data_sz cc_get_receive_data_len(cc_buffer_desc *buf);

/*
 * Establish a secure connection with the cloud services. This must be called
 * before any transactions take place over the network. Every call to this
 * function must be paired with a call to cc_close_session().
 *
 * Parameters:
 * 	host : A NULL terminated string specifying the host server.
 * 	port : A NULL terminated string specifying the host port.
 *
 * Returns:
 * 	True  : A secure connection has been established.
 * 	False : Failed to establish a secure connection.
 */
bool cc_establish_session(const char *host, const char *port);

/*
 * Close the connection to the cloud services. Call this function to gracefully
 * end communications with the cloud services. This function must be paired with
 * every call to cc_establish_session().
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	None
 */
void cc_close_session(void);

/*
 * Send bytes to the cloud. The data is wrapped in protocol headers and sent
 * over a secure channel. A send is said to be active when it is waiting for a
 * response from the cloud services. Only one send can be active at a time.
 *
 * Parameters:
 * 	buf  : Cloud communication buffer descriptor containing the data to be sent.
 * 	sz   : Size of the data in bytes.
 * 	cb   : Pointer to the callback routine that will be invoked with the
 * 	       status of the send.
 *
 * Returns:
 * 	CC_SEND_FAILED  : Failed to send the message.
 * 	CC_SEND_BUSY    : A send is in progress.
 * 	CC_SEND_SUCCESS : Message was sent, waiting for a response from the cloud.
 */
cc_send_result cc_send_bytes_to_cloud(const cc_buffer_desc *buf, cc_data_sz sz,
		cc_callback_rtn cb);

/*
 * Initiate an asynchronous receive of bytes from the cloud. Only one receive
 * can be in progress at a time.
 *
 * Parameters:
 * 	buf  : Cloud communication buffer descriptor containing the data to be sent.
 * 	sz   : Size of the data in bytes.
 *
 * Returns:
 * 	CC_RECV_BUSY    : Can't initiate a receive since one is already in progress.
 * 	CC_RECV_SUCCESS : Successfully initiated a receive.
 */
cc_recv_result cc_recv_bytes_from_cloud(cc_buffer_desc *buf, cc_data_sz sz);

/*
 * Call this function periodically to advance the time-based activities related
 * to managing the connection to the cloud and the transactions on it. The CC
 * API does not have an internal timer to work with. Instead, this function
 * performs any pending activities and indicates when it next needs to be called
 * to service a time based event.
 *
 * Parameters:
 * 	current_timestamp : The current system tick time.
 *
 * Returns:
 * 	-1             : No time based calls are currently needed.
 * 	Positive value : Number of milliseconds after which this function should
 * 	                 be called again.
 */
int32_t cc_service_send_receive(int32_t current_timestamp);
#endif
