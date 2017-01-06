/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#ifndef __UART_H
#define __UART_H

#include <stdbool.h>
#include <stdint.h>

/*
 * UART driver.
 * This module provides access to the UART peripheral to communicate with the
 * modem through AT commands. The design of this driver draws from the
 * characteristics of the AT command set - there is an idle period after
 * receiving the response of an AT command.
 */

#define UART_RX_BUFFER_SIZE	1024

typedef uint16_t buf_sz;

#define UART_INV_PARAM	-1	/* Invalid parameter. */

#define UART_EN_HW_CTRL		true	/* Enable hardware flow control. */
#define UART_DIS_HW_CTRL	false	/* Disable hardware flow control. */

typedef enum callback_event {
	UART_EVENT_RECVD_BYTES,
	UART_EVENT_RX_OVERFLOW
} callback_event;

/*
 * The receive callback for the UART driver. This will be invoked whenever an
 * idle timeout of a previously configured number of characters is detected on
 * the line. The receive callback will also be called after a set fraction of
 * the buffer has been filled and when the internal buffer overflows.
 * The callback takes an event parameter that records the cause of the callback.
 */
typedef void (*uart_rx_cb)(callback_event event);

/*
 * Initialize the UART hardware with the following settings:
 * Baud rate       : 115200 bps
 * Data width      : 8 bits
 * Parity bits     : None
 * Stop bits       : 1
 * HW flow control : Configurable
 *
 * Also, configure the idle timeout delay in number of characters.
 * When the line remains idle for 't' characters after receiving the last
 * character, the driver invokes the receive callback.
 *
 * Parameters:
 *	flow_ctrl - Can take one of the following two values:
 *		UART_EN_HW_CTRL - Enable hardware flow control.
 *		UART_DIS_HW_CTRL - Disable hardware flow control.
 *
 * 	t - number of characters to wait. The actual duration is based on the
 * 	current UART baud rate.
 *
 * Returns:
 * 	True - If initialization was successful.
 * 	Fase - If initialization failed.
 */
bool uart_module_init(bool flow_ctrl, uint8_t t);

/*
 * Set the receive callback.
 *
 * Paramters:
 * 	cb - UART receive callback function pointer.
 *
 * Returns:
 * 	None
 */
void uart_set_rx_callback(uart_rx_cb cb);

/*
 * Send data over the UART. This is a blocking send. In case the modem blocks
 * the flow via hardware flow control, the call will block for at most 'timeout_ms'
 * milliseconds.
 *
 * Parameters:
 * 	data - Pointer to the data to be sent.
 * 	size - Number of bytes to send
 * 	timeout_ms - Total number of milliseconds to wait before giving up in
 * 	case of a busy channel.
 *
 * Returns:
 * 	True - If the data was sent out successfully.
 * 	False - Send aborted due to timeout or null pointer provided for data.
 */
bool uart_tx(uint8_t *data, buf_sz size, uint16_t timeout_ms);

/*
 * Return the number of unread bytes present in the buffer.
 *
 * Paramters:
 * 	None
 *
 * Returns:
 * 	Number of unread bytes in the buffer.
 */
buf_sz uart_rx_available(void);


/*
 * Utility function to scan internal uart ring buffer to find the pattern or
 * substring
 *
 * Parameters:
 * 	start_idx - starting index to being scan for the pattern, -1 is special
 *		    value where if supplied, it starts with begining of the ring
 *		    buffer
 * 	pattern - Null terminated string.
 *
 * Returns:
 * 	-1 if no such string is found inside the read buffer or parameters are
 *	wrong
 * 	On success, returns the starting position within the ring buffer of
 *	matched pattern
 */
int uart_find_pattern(int start_idx, const uint8_t *pattern, buf_sz nlen);

/*
 * Return the number of bytes that makes a complete line. A line is defined
 * as a string that begins with a specified header and ends with a specified
 * trailer.
 *
 * Parameters:
 * 	header - Null character terminated header string. If null, the header is
 * 	ignored and the count starts from the first read index.
 * 	trailer - Null character terminated trailer string. The trailer cannot
 * 	be empty.
 *
 * Returns:
 * 	Zero if no such string is found inside the read buffer.
 * 	On success, number of bytes the line comprises of including the header
 * 	and the	trailer bytes.
 * 	UART_INV_PARAM if the trailer is empty.
 */
int uart_line_avail(const char *header, const char *trailer);

/*
 * Retrieve a set of bytes from the UART's read buffer. The maximum number of
 * bytes that can be retrieved is given by the return value of 'uart_rx_available'.
 *
 * Parameters:
 * 	buf - Pointer to the buffer where the data will be read into.
 * 	sz - Maximum size the supplied buffer can store.
 *
 * Returns:
 * 	Number of bytes actually read into the buffer.
 * 	UART_INV_PARAM if null pointer is provided for buffer.
 */
int uart_read(uint8_t *buf, buf_sz sz);

/*
 * Flush the receive buffer. This resets the read and write head of the internal
 * buffer, effectively clearing the buffer.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	None
 */
void uart_flush_rx_buffer(void);

#endif
