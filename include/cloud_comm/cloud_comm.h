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
	CC_RECV_BUSY,
	CC_RECV_SUCCESS
} cc_recv_result;

typedef enum {
	CC_STS_ACK,		/* Received an ACK for the last message sent */
	CC_STS_NACK,		/* Received a NACK for the last message sent */
	CC_STS_SEND_TIMEOUT,	/* Timed out sending the message */
	CC_STS_RCV_CMD,		/* Received a command from the cloud */
	CC_STS_RCV_UPD		/* Received an update from the cloud */
} cc_event;

typedef uint16_t cc_data_sz;

typedef struct {
	cc_data_sz bufsz;
	void *buf_ptr;
} cc_buffer_desc;

#define __CC_RECV_BUFFER(name, max_sz) \
	union { \
		uint32_t cmd_value; \
		struct { \
			cc_data_sz sz;	/* Number of bytes filled in buffer */ \
			uint8_t bytes[(max_sz)]; \
		} data_array; \
	} name##__cc_buf; \
	cc_buffer_desc name = {(max_sz), &(name##__cc_buf)};

#define CC_SEND_BUFFER(name, max_sz) __CC_SEND_BUFFER(name, (max_sz))

#define CC_RECV_BUFFER(name, max_sz) __CC_RECV_BUFFER(name, (max_sz))

/*
 * Pointer to callback routine. The callback accepts a buffer descriptor and
 * an event from the source of the callback explaining why the callback was
 * invoked.
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

bool cc_send_in_progress(void);

uint8_t *cc_get_send_buffer_ptr(cc_buffer_desc *buf);

uint32_t cc_read_recv_data_as_int(cc_buffer_desc *buf);

uint8_t *cc_read_recv_data_as_byte_array(cc_buffer_desc *buf, cc_data_sz *sz);

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
cc_send_result cc_send_bytes_to_cloud(cc_buffer_desc *buf, cc_data_sz sz,
		cc_callback_rtn cb);

cc_recv_result cc_recv_bytes_from_cloud(cc_buffer_desc *buf, cc_data_sz sz.
		cc_callback_rtn cb);

int32_t cc_service_send_receive(void);
#endif
