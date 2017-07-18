/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include "sys.h"
#include "uart_util.h"
#include "timer_hal.h"
#include "timer_interface.h"
#include "ts_sdk_board_config.h"

#define CALLBACK_TRIGGER_MARK	((buf_sz)(UART_BUF_SIZE * ALMOST_FULL_FRAC))
#define ALMOST_FULL_FRAC	0.6	/* Call the receive callback once this
					 * fraction of the buffer is full.
					 */
#define CEIL(x, y)		(((x) + (y) - 1) / (y))
#define MICRO_SEC_MUL		1000000
#define TIM_BASE_FREQ_HZ	1000000
#define TIMEOUT_BYTE_US		CEIL((8 * MICRO_SEC_MUL), MODEM_UART_BAUD_RATE)

static uart_rx_cb recv_callback;
static periph_t uart;

#define INVOKE_CALLBACK(ev)	do { \
	if (recv_callback) \
		recv_callback(ev); \
} while (0)

/* The internal buffer that will hold incoming data. */
static volatile struct {
	uint8_t buffer[UART_BUF_SIZE];
	buf_sz ridx;
	buf_sz widx;
	buf_sz num_unread;
} rx;

/* Store the idle timeout in number of characters. */
static uint16_t timeout_chars;

static const timer_interface_t *idle_timer;
static volatile uint16_t num_idle_chars;
static volatile bool byte_recvd;	/* Set if a byte was received in the
					 * last TIMEOUT_BYTE_US microseconds.
					 */

static void rx_char_cb(uint8_t data)
{
	byte_recvd = true;

	/* If the timer isn't running, this is the first byte of the
	response. */
	if (!timer_is_running(idle_timer)) {
		num_idle_chars = 0;
		timer_start(idle_timer);
	}

	/* Buffer characters as long as the size of the buffer isn't
	exceeded. */
	if (rx.num_unread < UART_BUF_SIZE) {
		rx.buffer[rx.widx] = data;
		rx.widx = (rx.widx + 1) % UART_BUF_SIZE;
		rx.num_unread++;
		dsb();
	} else {
		INVOKE_CALLBACK(UART_EVENT_RX_OVERFLOW);
	}
}

static void idle_timer_callback(void)
{
	/*
	 * If a byte was not received in the last timeout period, increment the
	 * number of idle characters seen so far. Otherwise, reset the number of
	 * idle characters.
	 */
	if (byte_recvd) {
		byte_recvd = false;
		num_idle_chars = 0;

	} else {
		num_idle_chars++;
	}
	/*
	* Invoke the callback with the receive event in two situations:
	* > When the RX line is detected to be idle after a response has
	* begun arriving.
	* > When a certain percentage of bytes have been received.
	*/
	if ((num_idle_chars >= timeout_chars) ||
			(rx.num_unread == CALLBACK_TRIGGER_MARK)) {
		timer_stop(idle_timer);
		INVOKE_CALLBACK(UART_EVENT_RECVD_BYTES);
	}
}

static bool idle_timer_init(void)
{
	/*
	 * Initialize the timer (TIM2) to have a period equal to the time it
	 * takes to receive a character at the current baud rate.
	 * The clock is set to restart every 70us (value of 'Period' member)
	 * which is how much time it takes to receive a character on a 115200
	 * bps UART.
	 */

	idle_timer = timer_get_interface(MODEM_UART_IDLE_TIMER);
	if (!idle_timer)
		return false;
	return timer_init(idle_timer, TIMEOUT_BYTE_US - 1, IDL_TIM_IRQ_PRIORITY,
			TIM_BASE_FREQ_HZ, idle_timer_callback);
}

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

	uart_set_rx_char_cb(uart, rx_char_cb);

	if (idle_timer_init() == false)
		return false;

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
	} while (bidx != rx.widx);		/* Scan until write index */
	return -1;				/* No substring found */
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
	uart_irq_off(uart);
	buf_sz num_unread = rx.num_unread;
	uart_irq_on(uart);

	buf_sz n_bytes = (sz > num_unread) ? num_unread : sz;
	buf_sz i = 0;
	while ((n_bytes - i) > 0) {
		buf[i++] = rx.buffer[rx.ridx];
		rx.ridx = (rx.ridx + 1) % UART_BUF_SIZE;
	}

	uart_irq_off(uart);
	rx.num_unread -= n_bytes;
	uart_irq_on(uart);

	return n_bytes;
}

void uart_util_flush(void)
{
	rx.widx = 0;
	rx.ridx = 0;
	rx.num_unread = 0;
}
