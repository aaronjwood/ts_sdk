/**
 * \file net.h
 *
 * \brief AT command functions to talk to LTE Modems
 *
 * \copyright Copyright (C) 2016, Verizon. All rights reserved.
 *
 *
 */

#ifndef OTT_AT_H
#define OTT_AT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** tcp read/write api related error codes */
/** send failure */
#define AT_TCP_SEND_FAIL   -1
/** receive failure */
#define AT_TCP_RCV_FAIL    -1
#define AT_TCP_INVALID_PARA -3

/** connect dropped may be between connect and read or write or in middle of
 * write
 */
#define AT_TCP_CONNECT_DROPPED  -2

/** TCP connect related error codes */
#define AT_CONNECT_FAILED       -1
#define AT_SOCKET_FAILED        -2

/**
 * \brief       Initializes underlying hardware i.e. UART etc...and modem
 * \return      true if successful or false if fails
 */
bool at_init();

/**
 * \brief       Initiate a tcp connection with host:port
 *
 * \param[in] host    Host to connect to
 * \param[in] port    Port to connect to
 *
 * \return      socket or session id number if successful or
 *              AT_CONNECT_FAILED/AT_SOCKET_FAILED for failure
 *
 */
int at_tcp_connect(const char *host, const char *port);

/**
 * \brief          Write at most 'len' characters. If no error occurs,
 *                 the actual amount written from buf is returned.
 *
 * \param[in] s_id    Socket or session id to send to
 * \param[in] buf     The buffer to read from
 * \param[in] len     The length of the buffer
 *
 * \return         The number of bytes sent,
 *                 or AT_TCP_SEND_FAIL/AT_TCP_CONNECT_DROPPED if fails;
 */
int at_tcp_send(int s_id, const uint8_t *buf, size_t len);

/**
 * \brief               query AT layer if data is available to read
 *
 * \param[in] s_id      Socket or session id to read
 * \return              number of bytes of data available to read,
 *                      AT_TCP_RCV_FAIL/AT_TCP_CONNECT_DROPPED for any
 *                      other errors, for example, socket is not valid
 *
 */
int at_read_available(int s_id);

/**
 * \brief          Read at most 'len' characters.
 *                 If no error occurs, the actual amount read is returned.
 *
 * \param[in] s_id      Socket or session id to read
 * \param[out] buf      The buffer to write to
 * \param[in] len       Maximum length of the buffer
 *
 * \return         The number of bytes received,
 *                 or AT_TCP_RCV_FAIL/AT_TCP_CONNECT_DROPPED for error
 *
 */
int at_tcp_recv(int s_id, uint8_t *buf, size_t len);

/**
 * \brief               Gracefully shutdown the connection
 *
 * \param[in] s_id      socket or tcp session id to close to
 */
void at_tcp_close(int s_id);

#ifdef __cplusplus
}
#endif

#endif /* at.h */
