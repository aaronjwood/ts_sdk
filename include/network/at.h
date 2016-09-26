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


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Initializes underlying hardware i.e. UART etc...and modem
 * \return true if successful or false if fails
 */
bool at_init();

/**
 * \brief       Initiate a tcp connection with host:port
 *
 * \param[in] host    Host to connect to
 * \param[in] port    Port to connect to
 *
 * \return      socket or session id number if successful or -1 for failure
 *
 */
int at_tcp_connect(const char *host, int port);

/**
 * \brief          Write at most 'len' characters. If no error occurs,
 *                 the actual amount written from buf is returned.
 *
 * \param[in] s_id    Socket or session id to send to
 * \param[in] buf     The buffer to read from
 * \param[in] len     The length of the buffer
 *
 * \return         The number of bytes sent,
 *                 or -1 if fails;
 */
int at_tcp_send(int s_id, const unsigned char *buf, size_t len);

/**
 * \brief          Read at most 'len' characters, blocking for at most
 *                 'timeout' seconds. If no error occurs, the actual amount
 *                 read is returned.
 *
 * \param[in] s_id      Socket or session id to read
 * \param[out] buf      The buffer to write to
 * \param[in] len       Maximum length of the buffer
 * \param[in] timeout   Maximum number of milliseconds to wait for data
 *                      0 means wait forever
 *
 * \return         The number of bytes received,
 *                 or -1 for time out
 *
 */
int at_tcp_recv_timeout(int s_id, unsigned char *buf,
        size_t len, uint32_t timeout);

/**
 * \brief               Gracefully shutdown the connection
 *
 * \param[in] s_id      socket or tcp session id to close to
 */
int at_tcp_close(int s_id);

#ifdef __cplusplus
}
#endif

#endif /* at.h */
