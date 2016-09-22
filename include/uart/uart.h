/* Copyright(C) 2016 Verizon. All rights reserved. */

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

#define UART_READ_ERR		-1	/* Error while reading UART buffer. */
#define UART_EN_HW_CTRL		true	/* Enable hardware flow control. */
#define UART_DIS_HW_CTRL	false	/* Disable hardware flow control. */

typedef enum callback_event {
	UART_EVENT_RESP_RECVD,
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
 * Parameters:
 *	flow_ctrl - Can take one of the following two values:
 *		UART_EN_HW_CTRL - Enable hardware flow control.
 *		UART_DIS_HW_CTRL - Disable hardware flow control.
 *
 * Returns:
 * 	True - If initialization was successful.
 * 	Fase - If initialization failed.
 */
bool uart_module_init(bool flow_ctrl);

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
 * Configure the idle timeout delay in number of characters.
 * When the line remains idle for 't' characters after receiving the last
 * character, the driver invokes the receive callback.
 *
 * Paramters:
 * 	t - number of characters to wait. The actual duration is based on the
 * 	current UART baud rate.
 *
 * Returns:
 * 	True - If the new timeout value was successfully set.
 * 	False - Failed to set the new timeout value.
 */
bool uart_config_idle_timeout(uint8_t t);

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
bool uart_tx(uint8_t *data, uint16_t size, uint16_t timeout_ms);

/*
 * Return the number of unread bytes present in the buffer.
 *
 * Paramters:
 * 	None
 *
 * Returns:
 * 	Number of unread bytes in the buffer.
 */
uint16_t uart_rx_available(void);

/*
 * Retrieve a set of bytes from the UART's read buffer. The maximum number of
 * bytes that can be retrieved is given by the return value of 'uart_rx_available'.
 *
 * Parameters:
 * 	buf - Pointer to the buffer where the data will be read into.
 * 	sz - Size of the data (in bytes) to be read.
 *
 * Returns:
 * 	Number of bytes read from the buffer.
 * 	UART_READ_ERR if there's no unread data or 'sz' exceeds the number of
 * 	unread bytes or null pointer provided for buffer.
 */
int uart_read(uint8_t *buf, uint16_t sz);

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
