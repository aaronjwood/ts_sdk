/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include "dbg.h"
#include "ott_protocol.h"
/* Include AT Layer module: #include "AT_LAYER.H" */

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#ifdef MBEDTLS_DEBUG_C
#include "mbedtls/debug.h"
#endif

#define VERSION_BYTE		((uint8_t)0x01)

#if 0
#include "verizon_ott_ca.h"
#define SERVER_PORT "4433"
#define SERVER_NAME "localhost"
#else
/* Temporary - a site that happens to speak our single crypto suite. */
#include "dst_root_ca_x3.h"
#define SERVER_PORT "443"
#define SERVER_NAME "www.atper.org"
#endif

#ifdef MBEDTLS_DEBUG_C
static void my_debug(void *ctx, int level,
                     const char *file, int line,
		     const char *str)
{
	UNUSED(level);
	dbg_printf("%s:%04d: %s", file, line, str);
	fflush(stdout);
}
#endif

static uint8_t msg_buf[MAX_MSG_SZ];
static bool message_in_buffer;		/* Set when a message is received. */

/* mbedTLS specific variables */
static mbedtls_net_context server_fd;
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_ssl_context ssl;
static mbedtls_ssl_config conf;
static mbedtls_x509_crt cacert;

static inline void cleanup_mbedtls(void)
{
	/* Free network interface resources. */
	mbedtls_net_free(&server_fd);
	mbedtls_x509_crt_free(&cacert);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
}

ott_status ott_protocol_init(void)
{
	int ret;
	message_in_buffer = false;

	/* XXX: Initialize the AT layer here */

#ifdef MBEDTLS_DEBUG_C
	mbedtls_debug_set_threshold(1);
#endif
	mbedtls_net_init(&server_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	/* Seed the RNG */
	mbedtls_entropy_init(&entropy);
	ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
			NULL, 0);
	if (ret != 0)
		return OTT_ERROR;

	/* Load the CA root certificate */
	ret = mbedtls_x509_crt_parse_der(&cacert_der,
                                         (const unsigned char *)&cacert_der,
					 sizeof(cacert_der));
	if (ret < 0)
		return OTT_ERROR;

	return OTT_OK;
}

ott_status ott_initiate_connection(void)
{
	int ret;

	/* Connect to the cloud server over TCP */
	ret = mbedtls_net_connect(&server_fd, SERVER_NAME, SERVER_PORT,
			MBEDTLS_NET_PROTO_TCP);
	if (ret < 0)
		return OTT_ERROR;

	/* Set up the TLS structures */
	ret = mbedtls_ssl_config_defaults(&conf,
			MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0)
		return OTT_ERROR;

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_VERIFY_REQUIRED);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef MBEDTLS_DEBUG_C
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
#endif
	ret = mbedtls_ssl_setup(&ssl, &conf);
	if (ret != 0)
		return OTT_ERROR;

	/* This handles non-blocking I/O and will poll for completion. */
	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send,
			mbedtls_net_recv, NULL);

	/* Perform TLS handshake */
	ret = mbedtls_ssl_handshake(&ssl);
	while (ret != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE)
			return OTT_ERROR;
		ret = mbedtls_ssl_handshake(&ssl);
	}

	return OTT_OK;
}

ott_status ott_close_connection(void)
{
	return ((mbedtls_ssl_close_notify(&ssl) == 0) ? OTT_OK : OTT_ERROR);
}

ott_status ott_send_auth_to_cloud(c_flags_t c_flags, const uuid_t dev_id,
				  uint16_t dev_sec_sz, const uint8_t *dev_sec)
{
	/* Both ACK and NACK flags cannot be set at once. */
	if ((c_flags & (CF_NACK | CF_ACK) == (CF_NACK | CF_ACK)) ||
			(dev_sec_sz + UUID_SZ > MAX_DATA_SZ) ||
			(dev_id == NULL || dev_sec == NULL || dev_sec_sz == 0))
		return OTT_INV_PARAM;

	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_send_status_to_cloud(c_flags_t c_flags,
                                    uint16_t status_sz,
				    const uint8_t *status)
{
	/* Both ACK and NACK flags cannot be set at once. */
	if ((c_flags & (CF_NACK | CF_ACK) == (CF_NACK | CF_ACK)) ||
			(status_sz > MAX_DATA_SZ) ||
			(status == NULL))
		return OTT_INV_PARAM;

	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_send_ctrl_msg(c_flags_t c_flags)
{
	/* Both ACK and NACK flags cannot be set at once. */
	if ((c_flags & (CF_NACK | CF_ACK) == (CF_NACK | CF_ACK)) ||
			(c_flags == CF_PENDING))
		return OTT_INV_PARAM;

	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}

ott_status ott_retrieve_msg(msg_t *msg)
{
	/* TODO: Calls to mbedTLS API here: */

	return OTT_OK;
}
