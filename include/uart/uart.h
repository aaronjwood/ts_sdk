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

#define INV_DATA		-1	/* Signifies no unread data in the RX buffer. */
#define ALMOST_FULL_FRAC	0.6	/* Call the receive callback once this
					 * fraction of the buffer is full.
					 */

typedef void (*uart_rx_cb)(void);

/*
 * Initialize the UART hardware with the following settings:
 * Baud rate       : 115200 bps
 * Data width      : 8 bits
 * Parity bits     : None
 * Stop bits       : 1
 * HW flow control : Yes
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	None
 */
bool uart_module_init(void);

/*
 * Set the receive callback. This will be invoked whenever an idle timeout of
 * a previously configured number of characters is detected on the line.
 * The receive callback will also be called after a set fraction of the buffer
 * has been filled (see ALMOST_FULL_FRAC).
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
 * 	None
 */
void uart_config_idle_timeout(uint8_t t);

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
 * 	False - Send aborted due to timeout.
 */
bool uart_tx(uint8_t *data, uint16_t size, uint16_t timeout_ms);

/*
 * Check if there are still bytes available to be read in the driver's read
 * buffer.
 *
 * Paramters:
 * 	None
 *
 * Returns:
 * 	True - When there are unread bytes in the read buffer of the driver.
 * 	False - No bytes available in the read buffer.
 */
bool uart_rx_available(void);

/*
 * Retrieve a byte from the UART's read buffer.
 *
 * Parameters:
 * 	None
 *
 * Returns:
 * 	A byte if there is unread data in the read buffer.
 * 	INV_DATA if there's no unread data.
 */
int uart_read(void);

/*
 * This function is used to detect idle timeouts on the UART RX line. An idle
 * timeout is used to signify the end of a response from the modem. It is
 * expected that this function be called at 1ms intervals to check for an idle
 * line. If the line is idle, the receive callback supplied by the user is
 * invoked.
 *
 * Parameters:
 * 	None
 *
 * Returns
 * 	None
 */
void uart_handle_idle_timeout(void);

#endif
