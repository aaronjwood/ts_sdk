/*
 *  \file net.c
 *
 *  \brief net functions for mbedtls library
 *
 *  \copyright Copyright (C) 2016, Verizon. All rights reserved.
 *
 */

//Dipen: FIXME: enable with mbedtls
#if 0
#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif
#endif

#include <stm32f4xx_hal.h>
#include "at.h"
#include "net.h"
#include <stdio.h>
#include <stdint.h>


#define read(fd, buf, len)      at_tcp_recv(fd, (uint8_t *)buf, (size_t)len)
#define write(fd, buf, len)     at_tcp_send(fd, (uint8_t *)buf, (size_t)len)
#define close(fd)               at_tcp_close(fd)

/*
 * Initialize a context
 */
bool mbedtls_net_init(mbedtls_net_context *ctx)
{
        if (!ctx)
                return false;
        ctx->fd = -1;
        return at_init();
}

/*
 * Initiate a TCP connection with host:port and the given protocol
 */
int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host,
                        const char *port, int proto)
{
        int ret = 0;
        if (proto != MBEDTLS_NET_PROTO_TCP)
                return MBEDTLS_ERR_NET_SOCKET_FAILED;

        ret = at_tcp_connect(host, port);
        if (ret < 0)
                return MBEDTLS_ERR_NET_SOCKET_FAILED;
        ctx->fd = ret;
        return ret;
}

/*
 * Read at most 'len' characters
 */
int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len)
{
        int ret;
        int fd = ((mbedtls_net_context *) ctx)->fd;

        if (fd < 0)
                return MBEDTLS_ERR_NET_INVALID_CONTEXT;

        ret = read(fd, buf, len);
        if (ret < 0) {
                if (ret == AT_TCP_CONNECT_DROPPED) {
                        printf("%s: connection dropped\n", __func__);
                        return MBEDTLS_ERR_NET_CONN_RESET;
                }
                /* FIXME: Figure out if this is possible with AT layer
                if (errno == EINTR)
                        return MBEDTLS_ERR_SSL_WANT_READ;
                */

                return MBEDTLS_ERR_NET_RECV_FAILED;
        }
        return ret;
}

/*
 * Read at most 'len' characters, blocking for at most 'timeout' ms
 */
int mbedtls_net_recv_timeout(void *ctx, unsigned char *buf,
                                size_t len, uint32_t timeout)
{
        int ret;
        int fd = ((mbedtls_net_context *) ctx)->fd;

        if( fd < 0 )
                return MBEDTLS_ERR_NET_INVALID_CONTEXT;
        ret = at_read_available(fd);
        uint32_t start = HAL_GetTick();
        bool timeout_flag = false;
        while (1) {
                if ((HAL_GetTick() - start) > timeout) {
                        timeout_flag = true;
                        break;
                } else
                        ret = at_read_available(fd);
        }
        if (ret == AT_TCP_CONNECT_DROPPED) {
                printf("%s: connection dropped\n", __func__);
                return MBEDTLS_ERR_NET_CONN_RESET;
        } else if (ret == AT_TCP_RCV_FAIL)
                return MBEDTLS_ERR_NET_RECV_FAILED;

        /* This call will not block */
        return mbedtls_net_recv(ctx, buf, len);
}

/*
 * Write at most 'len' characters
 */
int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len)
{
        int ret;
        int fd = ((mbedtls_net_context *) ctx)->fd;

        if( fd < 0 )
                return MBEDTLS_ERR_NET_INVALID_CONTEXT;

        ret = write(fd, buf, len);
        if (ret < 0) {
                if (ret == AT_TCP_CONNECT_DROPPED) {
                        printf("%s: connection dropped\n", __func__);
                        return MBEDTLS_ERR_NET_CONN_RESET;
                }
                /* FIXME: Figure out if this is possible with AT layer
                if (errno == EINTR)
                        return MBEDTLS_ERR_SSL_WANT_READ;
                */

                return MBEDTLS_ERR_NET_SEND_FAILED;
        }
        return ret;
}

/*
 * Gracefully close the connection
 */
void mbedtls_net_free(mbedtls_net_context *ctx)
{
        if (ctx->fd == -1)
                return;
        close(ctx->fd);
        ctx->fd = -1;
}
