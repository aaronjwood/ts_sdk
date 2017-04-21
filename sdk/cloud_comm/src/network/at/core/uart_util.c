/* Copyright(C) 2017 Verizon. All rights reserved. */

#include "uart_util.h"
#include "timer_hal.h"
#include "string.h"

static uart_rx_cb recv_callback;
static periph_t uart;
//static timer_interface_t *timer;

#define INVOKE_CALLBACK(ev)	do { \
	if (recv_callback) \
		recv_callback(ev); \
} while(0)

/* The internal buffer that will hold incoming data. */
static volatile struct {
	uint8_t buffer[UART_BUF_SIZE];
	buf_sz ridx;
	buf_sz widx;
	buf_sz num_unread;
} rx;

/* Store the idle timeout in number of characters. */
static uint16_t timeout_chars;

static volatile uint16_t num_idle_chars;
static volatile bool byte_recvd;	/* Set if a byte was received in the last
					 * TIMEOUT_BYTE_US microseconds.
					 */

bool uart_util_init(periph_t hdl, uint8_t idle_timeout)
{
	if (idle_timeout == 0 || hdl == NO_PERIPH)
		return false;

	/* Reset internal buffer's read and write heads. */
	rx.ridx = 0;
	rx.widx = 0;
	rx.num_unread = 0;

	timeout_chars = idle_timeout;
	uart = hdl;
	/* TODO: Initialize 'timer' */

	return true;
}

void uart_util_reg_callback(uart_rx_cb cb)
{
	recv_callback = cb;
}

buf_sz uart_util_available(void)
{
	return rx.num_unread;
}

/*
 * Find the substring 'substr' inside the receive buffer between the index
 * supplied and write index. Return the starting position of the substring if
 * found. Otherwise, return -1.
 */
static int find_substr_in_ring_buffer(buf_sz idx_start, const uint8_t *substr,
					buf_sz nlen)
{
	if (!substr || nlen == 0)
		return -1;

	buf_sz bidx = idx_start;
	buf_sz idx = 0;
	buf_sz found_idx = idx_start;
	bool first_char_seen = false;
	do {
		if (substr[idx] == rx.buffer[bidx]) {	/* Bytes match */
			if (!first_char_seen) {
				first_char_seen = true;
				found_idx = bidx;
			}
			idx++;
			bidx++;
		} else {				/* Bytes mismatch */
			if (first_char_seen) {
				first_char_seen = false;
				bidx = found_idx + 1;
			} else
				bidx++;
			found_idx = idx_start;
			idx = 0;
		}
		if (first_char_seen && idx == nlen)	/* Substring found */
			return found_idx;
		if (bidx == UART_BUF_SIZE)	/* Index wrapping */
			bidx = 0;
	} while(bidx != rx.widx);			/* Scan until write index */
	return -1;					/* No substring found */
}

int uart_util_find_pattern(int start_idx, const uint8_t *pattern, buf_sz nlen)
{
	if ((start_idx >= UART_BUF_SIZE) || (!pattern) || (nlen == 0))
		return UART_BUF_INV_PARAM;
	if (start_idx == UART_BUF_BEGIN)
		start_idx = rx.ridx;
	return find_substr_in_ring_buffer(start_idx, pattern, nlen);
}

int uart_util_line_avail(const char *header, const char *trailer)
{
	if (!trailer)
		return UART_BUF_INV_PARAM;

	if (rx.num_unread == 0)
		return 0;

	int hidx = rx.ridx;		/* Store the header index */
	int tidx = 0;			/* Store the trailer index */
	buf_sz len = 0;
	buf_sz hlen = strlen(header);
	buf_sz tlen = strlen(trailer);

	if (rx.num_unread < hlen + tlen)
		return 0;

	if (header) {
		/* Search for the header from where we left off reading. */
		hidx = find_substr_in_ring_buffer(rx.ridx,
				(uint8_t *)header, hlen);
		if (hidx == -1)
			return 0;	/* Header specified but not found. */
	}

	/* If the header was found, skip the length of the header from the
	 * read location. This workaround is for the case where the header and
	 * the trailer are the same.
	 */
	tidx = find_substr_in_ring_buffer(rx.ridx + hlen,
			(uint8_t *)trailer, tlen);
	if (tidx == -1)			/* Trailer not found. */
		return 0;

	len = ((tidx >= hidx) ? (tidx - hidx) : (tidx + UART_BUF_SIZE - hidx));
	/* If there is a line, signified by (len != 0), adjust the length to
	 * include the trailer.
	 */
	len = (len == 0) ? len : len + tlen;
	return len;
}

int uart_util_read(uint8_t *buf, buf_sz sz)
{
	/*
	 * Return an error if there are no bytes to read in the buffer or if a
	 * null pointer was supplied in place of the buffer.
	 */
	if (rx.num_unread == 0)
		return 0;

	if (!buf)
		return UART_BUF_INV_PARAM;

	/*
	 * Copy bytes into the supplied buffer and perform the necessary
	 * book-keeping.
	 */
	uart_toggle_irq(uart, false);
	buf_sz num_unread = rx.num_unread;
	uart_toggle_irq(uart, true);

	buf_sz n_bytes = (sz > num_unread) ? num_unread : sz;
	buf_sz i = 0;
	while ((n_bytes - i) > 0) {
		buf[i++] = rx.buffer[rx.ridx];
		rx.ridx = (rx.ridx + 1) % UART_BUF_SIZE;
	}

	uart_toggle_irq(uart, false);
	rx.num_unread -= n_bytes;
	uart_toggle_irq(uart, true);

	return n_bytes;
}

void uart_util_flush(void)
{
	rx.widx = 0;
	rx.ridx = 0;
	rx.num_unread = 0;
}
