/**
 * \file net.h
 *
 * \brief Network communication functions
 *
 * \copyright Copyright (C) 2016, Verizon. All rights reserved.
 *
 *
 */

#ifndef OTT_NET_H
#define OTT_NET_H

#if !defined(MBEDTLS_CONFIG_FILE)
#include "config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include "ssl.h"

#include <stddef.h>
#include <stdint.h>

#define MBEDTLS_ERR_NET_SOCKET_FAILED   -0x0042 /**< Failed to open a socket */
#define MBEDTLS_ERR_NET_CONNECT_FAILED  -0x0044 /**< The connection to the given
                                                server / port failed. */
#define MBEDTLS_ERR_NET_RECV_FAILED     -0x004C  /**< Reading information from
                                                the socket failed. */
#define MBEDTLS_ERR_NET_SEND_FAILED     -0x004E  /**< Sending information
                                                through the socket failed. */
#define MBEDTLS_ERR_NET_CONN_RESET      -0x0050  /**< Connection was reset by peer. */
#define MBEDTLS_ERR_NET_INVALID_CONTEXT -0x0045  /**< The context is invalid,
                                                eg because it was free()ed. */

#define MBEDTLS_NET_PROTO_TCP 0 /**< The TCP transport protocol */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Wrapper type network context.
 *
 */
typedef struct {
    int fd;     /**< The underlying socket*/
} mbedtls_net_context;

/**
 * Makes the context ready to be used. Also, checks network status.
 * For example, in case of modem, querries if modem is initialized properly and
 * ready to use
 *
 * \param[out] ctx       Context to initialize
 * \return      true if successful, false otherwise
 */
bool mbedtls_net_init(mbedtls_net_context *ctx);

/**
 * \brief       Initiate a tcp connection with host:port
 *
 * \param[out] ctx     sets the socket number if successful
 * \param[in] host    Host to connect to
 * \param[in] port    Port to connect to
 * \param[in] proto   Only MBEDTLS_NET_PROTO_TCP is supported
 *
 * \return      0 if successful, or one of:
 *              MBEDTLS_ERR_NET_SOCKET_FAILED,
 *              MBEDTLS_ERR_NET_UNKNOWN_HOST,
 *              MBEDTLS_ERR_NET_CONNECT_FAILED
 *
 */
int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host,
                        const char *port, int proto);

/**
 * \brief          Write at most 'len' characters. If no error occurs,
 *                 the actual amount written from buf is returned.
 *
 * \param[in] ctx     Network context, generally socket
 * \param[in] buf     The buffer to read from
 * \param[in] len     The length of the buffer
 *
 * \return         the number of bytes sent, or a non-zero error code.
 */
int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len);

/**
 * \brief          Read at most 'len' characters. If no error occurs,
 *                 the actual amount read is returned.
 *
 * \param ctx[in]       Socket
 * \param buf[out]      The buffer to write to
 * \param len[in]       Maximum length of the buffer
 *
 * \return      The number of bytes received, or a non-zero error code.
 */
int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len);

/**
 * \brief          Read at most 'len' characters, blocking for at most
 *                 'timeout' seconds. If no error occurs, the actual amount
 *                 read is returned.
 *
 * \param[in] ctx       Network context, generally socket
 * \param[out] buf      The buffer to write to
 * \param[in] len       Maximum length of the buffer
 * \param[in] timeout   Maximum number of milliseconds to wait for data
 *                      0 means wait forever
 *
 * \return         the number of bytes received,
 *                 or a non-zero error code:
 *                 MBEDTLS_ERR_SSL_TIMEOUT if the operation timed out
 *
 *
 */
int mbedtls_net_recv_timeout(void *ctx, unsigned char *buf,
        size_t len, uint32_t timeout);

/**
 * \brief               Gracefully shutdown the connection and
 *                      free associated data.
 *
 * \param[in] ctx       The context to free
 */
void mbedtls_net_free( mbedtls_net_context *ctx );

#ifdef __cplusplus
}
#endif

#endif /* net.h */
