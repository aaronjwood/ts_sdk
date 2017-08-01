/* Copyright (C) 2016, 2017 Verizon. All rights reserved. */

/*
 *  \file net.c
 *
 *  \brief net functions for mbedtls library
 *
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include "at_tcp.h"
#include "mbedtls/net.h"
#include "sys.h"

#ifdef CALC_TLS_OVRHD_BYTES
bool ovrhd_profile_flag;
uint32_t num_bytes_recvd;
uint32_t num_bytes_sent;
#define INIT_COUNTERS()		do { \
	ovrhd_profile_flag = false; \
	num_bytes_recvd = 0; \
	num_bytes_sent = 0; \
} while (0)
#define ADD_TO_RECVD(x)		do { \
	if (ovrhd_profile_flag) \
		num_bytes_recvd += (x); \
} while (0)
#define ADD_TO_SEND(x)		do { \
	if (ovrhd_profile_flag) \
		(num_bytes_sent += (x)); \
} while (0)
#else
#define INIT_COUNTERS()
#define ADD_TO_RECVD(x)
#define ADD_TO_SEND(x)
#endif	/* CALC_TLS_OVRHD_BYTES */

#if defined(OTT_EXPLICIT_NETWORK_TIME) && defined(OTT_TIME_PROFILE)

uint32_t network_time_ms;
static uint32_t net_begin;

#define NET_TIME_PROFILE_BEGIN() \
	net_begin = sys_get_tick_ms()

#define NET_TIME_PROFILE_END() \
	network_time_ms += (sys_get_tick_ms() - net_begin)

#else

#define NET_TIME_PROFILE_BEGIN()
#define NET_TIME_PROFILE_END()

#endif	/* OTT_EXPLICIT_NETWORK_TIME && OTT_TIME_PROFILE */

#ifdef DEBUG_NET
#define DEBUG(...)              printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

#define CHECK_NULL(x, y) do { \
	if (!((x))) { \
		DEBUG("Fail at line: %d\n", __LINE__); \
		return ((y)); \
	} \
} while (0)

#define CHECK_SUCCESS(x, y, z)	\
	do { \
		if ((x) != (y)) { \
			DEBUG("Fail at line: %d\n", __LINE__); \
			return (z); \
		} \
	} while (0)

#define read(fd, buf, len)      at_tcp_recv(fd, (uint8_t *)buf, (size_t)len)
#define write(fd, buf, len)     at_tcp_send(fd, (uint8_t *)buf, (size_t)len)
#define close(fd)               at_tcp_close(fd)

/* flag to indicate if init is done successfully */
static bool init_flag;

/*
 * Initialize a context
 */
void mbedtls_net_init(mbedtls_net_context *ctx)
{
	INIT_COUNTERS();
	NET_TIME_PROFILE_BEGIN();
	init_flag = false;
	if (!ctx)
		return;
	ctx->fd = -1;
	if (at_init())
		init_flag = true;
	NET_TIME_PROFILE_END();
}

/*
 * Initiate a TCP connection with host:port and the given protocol
 */
int mbedtls_net_connect(mbedtls_net_context *ctx, const char *host,
		const char *port, int proto)
{
	NET_TIME_PROFILE_BEGIN();
	CHECK_NULL(ctx, MBEDTLS_ERR_NET_INVALID_CONTEXT);
	CHECK_NULL(host, MBEDTLS_ERR_NET_SOCKET_FAILED);
	CHECK_SUCCESS(init_flag, true, MBEDTLS_ERR_NET_SOCKET_FAILED);

	int ret = 0;
	if (proto != MBEDTLS_NET_PROTO_TCP)
		return MBEDTLS_ERR_NET_SOCKET_FAILED;

	ret = at_tcp_connect(host, port);
	if (ret == AT_CONNECT_FAILED)
		return MBEDTLS_ERR_NET_CONNECT_FAILED;
	else if (ret == AT_SOCKET_FAILED)
		return MBEDTLS_ERR_NET_SOCKET_FAILED;

	ctx->fd = ret;
	NET_TIME_PROFILE_END();
	return 0;
}

/*
 * Read at most 'len' characters
 */
int mbedtls_net_recv(void *ctx, unsigned char *buf, size_t len)
{
	NET_TIME_PROFILE_BEGIN();
	CHECK_NULL(ctx, MBEDTLS_ERR_NET_INVALID_CONTEXT);
	CHECK_NULL(buf, MBEDTLS_ERR_NET_INVALID_CONTEXT);
	CHECK_SUCCESS(init_flag, true, MBEDTLS_ERR_NET_INVALID_CONTEXT);

	int ret;
	int fd = ((mbedtls_net_context *) ctx)->fd;

	if (fd < 0)
		return MBEDTLS_ERR_NET_INVALID_CONTEXT;

	ret = read(fd, buf, len);
	if (ret < 0) {
		if (ret == AT_TCP_CONNECT_DROPPED) {
			DEBUG("%s: connection dropped\n", __func__);
			NET_TIME_PROFILE_END();
			return MBEDTLS_ERR_SSL_WANT_READ;
		}
		if (errno == EPIPE || errno == ECONNRESET) {
			NET_TIME_PROFILE_END();
			return MBEDTLS_ERR_NET_CONN_RESET;
		}

		if (errno == EINTR || errno == EAGAIN) {
			NET_TIME_PROFILE_END();
			return MBEDTLS_ERR_SSL_WANT_READ;
		}
		return MBEDTLS_ERR_NET_RECV_FAILED;
	}
	NET_TIME_PROFILE_END();
	ADD_TO_RECVD(ret);
	return ret;
}

/*
 * Read at most 'len' characters, blocking for at most 'timeout' ms
 */
int mbedtls_net_recv_timeout(void *ctx, unsigned char *buf,
		size_t len, uint32_t timeout)
{
	CHECK_NULL(ctx, MBEDTLS_ERR_NET_INVALID_CONTEXT);
	CHECK_NULL(buf, MBEDTLS_ERR_NET_INVALID_CONTEXT);
	CHECK_SUCCESS(init_flag, true, MBEDTLS_ERR_NET_INVALID_CONTEXT);

	int ret;
	int fd = ((mbedtls_net_context *) ctx)->fd;

	if (fd < 0)
		return MBEDTLS_ERR_NET_INVALID_CONTEXT;
	ret = at_read_available(fd);
	uint32_t start = sys_get_tick_ms();
	bool timeout_flag = false;
	while (ret <= 0) {
		if ((sys_get_tick_ms() - start) > timeout) {
			timeout_flag = true;
			break;
		} else
			ret = at_read_available(fd);
	}

	if (ret == AT_TCP_CONNECT_DROPPED) {
		DEBUG("%s: connection dropped\n", __func__);
		return MBEDTLS_ERR_NET_CONN_RESET;
	} else if (ret == AT_TCP_RCV_FAIL)
		return MBEDTLS_ERR_NET_RECV_FAILED;
	else if ((ret == 0) && timeout_flag)
		return MBEDTLS_ERR_SSL_TIMEOUT;

	/* This call will not block */
	return mbedtls_net_recv(ctx, buf, len);
}

/*
 * Write at most 'len' characters
 */
int mbedtls_net_send(void *ctx, const unsigned char *buf, size_t len)
{
	NET_TIME_PROFILE_BEGIN();
	CHECK_NULL(ctx, MBEDTLS_ERR_NET_INVALID_CONTEXT);
	CHECK_NULL(buf, MBEDTLS_ERR_NET_INVALID_CONTEXT);
	CHECK_SUCCESS(init_flag, true, MBEDTLS_ERR_NET_INVALID_CONTEXT);

	int ret;
	int fd = ((mbedtls_net_context *) ctx)->fd;

	if (fd < 0)
		return MBEDTLS_ERR_NET_INVALID_CONTEXT;

	ret = write(fd, buf, len);
	if (ret < 0) {
		if (ret == AT_TCP_CONNECT_DROPPED) {
			DEBUG("%s: connection dropped\n", __func__);
			NET_TIME_PROFILE_END();
			return MBEDTLS_ERR_SSL_WANT_WRITE;
		}
		if (errno == EPIPE || errno == ECONNRESET) {
			NET_TIME_PROFILE_END();
			return MBEDTLS_ERR_NET_CONN_RESET;
		}

		if (errno == EINTR) {
			NET_TIME_PROFILE_END();
			return MBEDTLS_ERR_SSL_WANT_WRITE;
		}

		return MBEDTLS_ERR_NET_SEND_FAILED;
	}
	NET_TIME_PROFILE_END();
	ADD_TO_SEND(ret);
	return ret;
}

/*
 * Gracefully close the connection
 */
void mbedtls_net_free(mbedtls_net_context *ctx)
{
	NET_TIME_PROFILE_BEGIN();
	if (!ctx)
		return;
	if (ctx->fd == -1)
		return;
	close(ctx->fd);
	ctx->fd = -1;
	NET_TIME_PROFILE_END();
}
