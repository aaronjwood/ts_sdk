/**
 * \file uart_buf.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Defines the UART buffer API.
 * \details This API implements a buffer and routines to access and manage
 * the buffer. It was specifically designed to be useful in talking to the AT
 * interface of a modem.
 */
#ifndef UART_BUF_H
#define UART_BUF_H

#include <stdint.h>
#include <stdbool.h>

/*
 * \brief Type that is wide enough to hold the size of UART related buffers.
 */
typedef uint16_t buf_sz;

/**
 * \brief Maximum size of the UART receive buffer.
 */
#define UART_BUF_SIZE		1024

/**
 * \brief Return value that represents an invalid parameter.
 */
#define UART_BUF_INV_PARAM	-2

/**
 * \brief Refers to the beginning of the buffer.
 */
#define UART_BUF_BEGIN		-1

/**
 * \brief Return value that indicates a failed search.
 */
#define UART_BUF_NOT_FOUND	-1

/**
 * \brief Defines events that cause the receive callback to be invoked.
 */
typedef enum callback_event {
	UART_EVENT_RECVD_BYTES,	/**< Callback was invoked on reception of a
				  chunk of bytes. */
	UART_EVENT_RX_OVERFLOW	/**< Callback was invoked since the underlying
				  buffer overflowed. */
} callback_event;

/**
 * \brief The receive callback for the UART buffer.
 * \details This will be invoked whenever the UART's receive line is detected
 * to be idle after receiving some bytes. The detection is carried out through
 * an idle timeout of a previously configured number of characters. The receive
 * callback will also be called after a set fraction of the buffer has been
 * filled or when the internal buffer overflows. The callback takes an event
 * parameter that records the cause of the callback.
 */
typedef void (*uart_rx_cb)(callback_event event);

/**
 * \brief Initialize buffer management module.
 * \details This routine will initialize the underlying receive buffer and set
 * a timeout to detect the idle state of the UART receive line. The initialization
 * routine must be called once before calling other routines in this module.
 * \param[in] idle_timeout Number of blank characters after which the UART is
 * deemed to be in an idle state.
 * \retval true Buffer was successfully initialized.
 * \retval false Failed to initialize buffer.
 */
bool uart_buf_init(uint8_t idle_timeout);

/**
 * \brief Set the receive callback.
 * \details When the UART receive line is detected to be idle, the registered
 * callback routine is invoked.
 * \param[in] cb Pointer to the receive callback
 * \pre \ref uart_buf_init must have been called once.
 */
void uart_buf_reg_callback(uart_rx_cb cb);

/**
 * \brief Return the number of unread bytes present in the buffer.
 * \returns Number of unread bytes in the buffer.
 * \pre \ref uart_buf_init must have been called once.
 */
buf_sz uart_buf_available(void);

/**
 * \brief Scan the buffer to find the pattern / substring.
 * \param[in] start_idx Starting index at which the scan should begin.
 * \b UART_BUF_BEGIN can be used to specify the beginning of the buffer.
 * \param[in] pattern Null terminated string to look for inside the buffer.
 * \param[in] nlen Length of the subset of the pattern to be matched.
 *
 * \retval UART_BUF_NOT_FOUND The pattern / subset of the pattern was not found
 * in the buffer.
 * \retval UART_BUF_INV_PARAM Invalid parameters were passed to this routine.
 * \retval >=0 Start index of the matched pattern within the buffer.
 * \pre \ref uart_buf_init must have been called once.
 */
int uart_buf_find_pattern(int start_idx, const uint8_t *pattern, buf_sz nlen);

/**
 * \brief Search for a complete line in the buffer.
 * \details A line is defined as a string that begins with a specified header
 * and ends with a specified trailer.
 *
 * \param[in] header Null terminated string representing the header. If \b NULL,
 * the header is ignored and the scan starts from the first read index.
 * \param[in] trailer Null terminated string representing the trailer. The trailer
 * cannot be empty.
 *
 * \retval UART_BUF_NOT_FOUND No line was found in the buffer.
 * \retval UART_INV_PARAM Trailer is empty
 * \retval >0 Number of bytes the line comprises of including the bytes in the
 * header and the trailer.
 * \pre \ref uart_buf_init must have been called once.
 */
int uart_buf_line_avail(const char *header, const char *trailer);

/**
 * \brief Retrieve a set of bytes from the buffer.
 * \details The maximum number of bytes that can be retrieved is given by the
 * return value of \ref uart_buf_available.
 *
 * \param[in] buf Pointer to the buffer where the data will be read into.
 * \param[in] sz Maximum size the supplied buffer (buf) can store.
 *
 * \retval UART_BUF_INV_PARAM Null pointer provided for buf.
 * \retval >=0 Number of bytes actually read into the buffer.
 * \pre \ref uart_buf_init must have been called once.
 */
int uart_buf_read(uint8_t *buf, buf_sz sz);

/**
 * \brief Flush the buffer.
 * \details This resets the read and write heads of the buffer, effectively
 * clearing it.
 * \pre \ref uart_buf_init must have been called once.
 */
void uart_buf_flush(void);

#endif
