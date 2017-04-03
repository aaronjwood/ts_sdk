/* Copyright(C) 2016 Verizon. All rights reserved. */

/*
 * A minimal test program to test the mbed TLS library.
 * Sets up a TLS connection to a web server and GET's the default page.
 *
 * Based loosly on the mbed TLS ssl_client1.c example.
 */
#ifndef BUILD_TARGET_OSX
#include "platform.h"
#else
#include <stdlib.h>
#endif

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#ifdef MBEDTLS_DEBUG_C
#include "mbedtls/debug.h"
#endif

#include <string.h>

#include "dbg.h"

#if 1
#include "verizon_ott_ca.h"
/*
 * The DevNet test server.
 * #define SERVER_NAME "testott.vzbi.com"
 * #define SERVER_PORT "443"
 */

/* The external test server */
#define SERVER_NAME "test1.ott.uu.net"
#define SERVER_PORT "443"

#else
/* Temporary - a site that happens to speak our single crypto suite. */
#include "dst_root_ca_x3.h"
#define SERVER_NAME "www.tapr.org"
#define SERVER_PORT "443"
#endif

#define GET_REQUEST "GET / HTTP/1.0\r\n\r\n"

#ifdef MBEDTLS_DEBUG_C
static void my_debug(void *ctx, int level,
		     const char *file, int line,
		     const char *str)
{
	((void) level);

	dbg_printf("%s:%04d: %s", file, line, str);
	fflush(stdout);
}
#endif

static void terminate(int ret)
{
	if (ret != 0)
		fatal_err("failed: %d\n", ret);

#ifdef BUILD_TARGET_OSX
	exit(ret);
#else
	while (1)	/* Don't exit on bare metal */
		;
#endif
}

int main(int argc, char *argv[])
{
	mbedtls_net_context server_fd;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;
	mbedtls_ssl_context ssl;
	mbedtls_ssl_config conf;
	mbedtls_x509_crt cacert;

	unsigned char buf[512];
	int ret;

#ifndef BUILD_TARGET_OSX
	platform_init();
#endif

	dbg_module_init();
	/*
	 * Note: It is assumed the modem has booted and is ready to receive
	 * commands at this point. If it isn't and HW flow control is enabled,
	 * the assertion over uart_tx is likely to fail implying the modem is
	 * not ready yet.
	 */
	dbg_printf("Initializing...\n");

#ifdef MBEDTLS_DEBUG_C
	mbedtls_debug_set_threshold(1);
#endif
	mbedtls_net_init(&server_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	dbg_printf("Seeding RNG...\n");
	mbedtls_entropy_init(&entropy);
	ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				    NULL, 0);
	if (ret != 0)
		goto done;

	dbg_printf("Loading CA root cert...\n");
	ret = mbedtls_x509_crt_parse_der(&cacert,
					 (const unsigned char *)&cacert_der,
					 sizeof(cacert_der));
	if (ret < 0)
		goto done;

	dbg_printf("Connecting to %s/%s...\n", SERVER_NAME, SERVER_PORT);
	ret = mbedtls_net_connect(&server_fd, SERVER_NAME, SERVER_PORT,
				  MBEDTLS_NET_PROTO_TCP);
	if (ret < 0)
		goto done;

	dbg_printf("Setting up TLS structure...\n");
	ret = mbedtls_ssl_config_defaults(&conf,
					  MBEDTLS_SSL_IS_CLIENT,
					  MBEDTLS_SSL_TRANSPORT_STREAM,
					  MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0)
		goto done;

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef MBEDTLS_DEBUG_C
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
#endif

	ret = mbedtls_ssl_setup(&ssl, &conf);
	if (ret != 0)
		goto done;

	/* This handles non-blocking I/O and will poll for completion below. */
	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send,
			    mbedtls_net_recv, NULL);

	/*
	 * Set the server identity (hostname) that must be present in its
	 * certificate CN or SubjectAltName.
	 */
	dbg_printf("Setting required server identity...\n");
	ret = mbedtls_ssl_set_hostname(&ssl, SERVER_NAME);
	if (ret != 0)
		goto done;
	   
	dbg_printf("Performing TLS handshake...\n");
	ret = mbedtls_ssl_handshake(&ssl);
	while (ret != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE)
			goto done;
		ret = mbedtls_ssl_handshake(&ssl);
	}

	dbg_printf("Writing to server...\n");
	int len = sprintf((char *)buf, GET_REQUEST);
	ret = mbedtls_ssl_write(&ssl, buf, len);
	while (ret <= 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
		    ret != MBEDTLS_ERR_SSL_WANT_WRITE)
			goto done;
		ret = mbedtls_ssl_write(&ssl, buf, len);
	}

	len = ret;
	dbg_printf("    Wrote %d bytes\n", len);

	dbg_printf("Reading response from server...\n");
	do {
		len = sizeof(buf) - 1;
		memset(buf, 0, sizeof(buf));
		ret = mbedtls_ssl_read(&ssl, buf, len);
		if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
		    ret == MBEDTLS_ERR_SSL_WANT_WRITE)
			continue;

		if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
			dbg_printf("Server closed connection\n");
			break;
		}

		if (ret < 0)
			goto done;

		if (ret == 0) {
			dbg_printf("EOF\n");
			break;
		} else
			dbg_printf("%s",(char *)buf);

	} while (1);

	mbedtls_ssl_close_notify(&ssl);
	ret = 0;

done:
	mbedtls_net_free(&server_fd);
	mbedtls_x509_crt_free(&cacert);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);

	terminate(ret);
	return 0;
}
