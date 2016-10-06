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

static unsigned char msg_buf[MAX_MSG_SZ];
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
	/* Initialize TLS structures */
	mbedtls_net_init(&server_fd);
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	/* Seed the RNG */
	mbedtls_entropy_init(&entropy);
	ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
			NULL, 0);
	if (ret != 0) {
		cleanup_mbedtls();
		return OTT_ERROR;
	}

	/* Load the CA root certificate */
	ret = mbedtls_x509_crt_parse_der(&cacert,
                                         (const unsigned char *)&cacert_der,
					 sizeof(cacert_der));
	if (ret < 0) {
		cleanup_mbedtls();
		return OTT_ERROR;
	}

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

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef MBEDTLS_DEBUG_C
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
#endif
	ret = mbedtls_ssl_setup(&ssl, &conf);
	if (ret != 0)
		return OTT_ERROR;

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send,
			mbedtls_net_recv, NULL);

	/* Perform TLS handshake */
	ret = mbedtls_ssl_handshake(&ssl);
	while (ret != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			return OTT_ERROR;
		}
		ret = mbedtls_ssl_handshake(&ssl);
	}

	return OTT_OK;
}

ott_status ott_close_connection(void)
{
	/* Close the connection and notify the peer. */
	int ret = mbedtls_ssl_close_notify(&ssl);
	if (ret == 0)
		return OTT_OK;
	mbedtls_ssl_session_reset(&ssl);
	return OTT_ERROR;
}

static bool write_tls(uint8_t *buf, uint16_t len)
{
	/* Attempt to write 'len' bytes of 'buf' over the TCP/TLS stream. */
	int ret = mbedtls_ssl_write(&ssl, (const unsigned char *)buf, (size_t)len);
	while (ret <= 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			return false;
		}
		ret = mbedtls_ssl_write(&ssl, (const unsigned char *)buf,
				(size_t)len);
	}
	return true;
}

/* Return "true" if the flag settings are valid. */
static bool flags_are_valid(c_flags_t f)
{
	/*
	 * Following are the invalid flag settings:
	 * NACK + ACK
	 * NACK + PENDING
	 * NACK + ACK + PENDING
	 * NACK + PENDING + QUIT
	 * NACK + ACK + QUIT
	 * NACK + ACK + PENDING + QUIT
	 *
	 * Accounting for the first two takes care of the rest.
	 */
	if (OTT_FLAG_IS_SET(f, CF_NACK | CF_ACK) ||
			OTT_FLAG_IS_SET(f, CF_NACK | CF_PENDING))
		return false;
	return true;
}

ott_status ott_send_auth_to_cloud(c_flags_t c_flags, const uuid_t dev_id,
				  uint16_t dev_sec_sz, const uint8_t *dev_sec)
{
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags) || (dev_sec_sz + UUID_SZ > MAX_DATA_SZ) ||
			(dev_id == NULL || dev_sec == NULL || dev_sec_sz == 0))
		return OTT_INV_PARAM;

	uint16_t idx = 0;
	uint16_t len = VER_SZ + CMD_SZ + UUID_SZ + LEN_SZ + dev_sec_sz;
	unsigned char buf[len];

	/* The version byte is sent before the very first message. */
	buf[0] = VERSION_BYTE;
	if (write_tls(buf, 1))
		return OTT_OK;
	else
		return OTT_ERROR;

	/* Build the rest of the message. */
	buf[idx++] = (uint8_t)(c_flags << 4 | MT_AUTH);

	for (uint16_t i = 0; i < UUID_SZ; i++)
		buf[idx++] = dev_id[i];

	buf[idx++] = (uint8_t)(dev_sec_sz & 0xFF);
	buf[idx++] = (uint8_t)((dev_sec_sz >> 8) & 0xFF);
	for (uint16_t i = 0; i < dev_sec_sz; i++)
		buf[idx++] = dev_sec[i];

	if (write_tls(buf, len))
		return OTT_OK;
	else
		return OTT_ERROR;
}

ott_status ott_send_status_to_cloud(c_flags_t c_flags,
                                    uint16_t status_sz,
				    const uint8_t *status)
{
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags) || (status_sz > MAX_DATA_SZ) ||
			(status == NULL))
		return OTT_INV_PARAM;

	uint16_t idx = 0;
	uint16_t len = CMD_SZ + LEN_SZ + status_sz;
	unsigned char buf[len];

	buf[idx++] = (uint8_t)(c_flags << 4 | MT_STATUS);
	buf[idx++] = (uint8_t)(status_sz & 0xFF);
	buf[idx++] = (uint8_t)((status_sz >> 8) & 0xFF);
	for (uint16_t i = 0; i < status_sz; i++)
		buf[idx++] = status[i];

	if (write_tls(buf, len))
		return OTT_OK;
	else
		return OTT_ERROR;
}

ott_status ott_send_ctrl_msg(c_flags_t c_flags)
{
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags) || OTT_FLAG_IS_SET(c_flags, CF_PENDING))
		return OTT_INV_PARAM;

	unsigned char buf = (uint8_t)(c_flags << 4 | MT_NONE);

	if (write_tls(&buf, 1))
		return OTT_OK;
	else
		return OTT_ERROR;
}

/* Return "false" if the message has invalid data. */
static bool populate_msg_struct(msg_t *msg)
{
	msg->c_flags = (c_flags_t)((msg_buf[0] >> 4) & 0x0F);
	msg->m_type = (m_type_t)(msg_buf[0] & 0x0F);
	switch (msg->m_type) {
	case MT_UPDATE:
		msg->array.sz = (uint16_t)((msg_buf[2] << 8) | msg_buf[1]);
		if (msg->array.sz >= MAX_DATA_SZ)
			return false;
		memcpy(msg->array.bytes, &msg_buf[3], msg->array.sz);
		break;
	case MT_CMD_PI:
	case MT_CMD_SL:
		msg->cmd_value = (uint32_t)(msg_buf[4] << 24 | msg_buf[3] << 16 |
					msg_buf[2] << 8 | msg_buf[1]);
	default:
		return false;
		break;
	}
	return true;
}

ott_status ott_retrieve_msg(msg_t *msg)
{
	if (msg == NULL)
		return OTT_INV_PARAM;

	int ret = mbedtls_ssl_read(&ssl, msg_buf, MAX_MSG_SZ);
	if (ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
			ret == MBEDTLS_ERR_SSL_WANT_READ ||
			ret == OTT_NO_MSG)
		return OTT_NO_MSG;

	if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
		if (ott_close_connection() != OTT_OK)
			return OTT_ERROR;
		return OTT_NO_MSG;
	}

	if (ret < 0) {
		mbedtls_ssl_session_reset(&ssl);
		return OTT_ERROR;
	}

	if (ret == MAX_MSG_SZ) {	/* Retrieved complete message */
		if (!populate_msg_struct(msg))
			return OTT_MSG_CORRUPT;
		return OTT_OK;
	}

	if (ret == 0)			/* No more messages to receive */
		return OTT_NO_MSG;

	return OTT_ERROR;
}

/* Stubs for now */
void mbedtls_net_init( mbedtls_net_context *ctx )
{
	(void)ctx;
}
int mbedtls_net_connect( mbedtls_net_context *ctx, const char *host, const char *port, int proto )
{
	(void)ctx; (void)host; (void)port; (void)proto;
	return 0;
}

void mbedtls_net_free( mbedtls_net_context *ctx )
{
	(void)ctx;
}

int mbedtls_net_recv( void *ctx, unsigned char *buf, size_t len )
{
	(void)ctx; (void)buf; (void)len;
	return -1;
}

int mbedtls_net_send( void *ctx, const unsigned char *buf, size_t len )
{
	(void)ctx; (void)buf; (void)len;
	return 0;
}
