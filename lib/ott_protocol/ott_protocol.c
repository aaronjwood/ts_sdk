/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
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
#define TIMEOUT_MS		5000

#if 0
#include "verizon_ott_ca.h"
#else
#include "dst_root_ca_x3.h"
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

static unsigned char msg_buf[OTT_MAX_MSG_SZ];	/* Stores received messages */

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

	/*
	 * XXX: Move this section to ott_initiate_connection() (after connect
	 * and before setting up the SSL context) if it doesn't work when placed
	 * here.
	 */
	/* Set up the TLS structures */
	ret = mbedtls_ssl_config_defaults(&conf,
			MBEDTLS_SSL_IS_CLIENT,
			MBEDTLS_SSL_TRANSPORT_STREAM,
			MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		cleanup_mbedtls();
		return OTT_ERROR;
	}

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef MBEDTLS_DEBUG_C
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
#endif

	return OTT_OK;
}

ott_status ott_initiate_connection(const char *host, const char *port)
{
	if (host == NULL || port == NULL)
		return OTT_INV_PARAM;

	int ret;
	/* Connect to the cloud server over TCP */
	ret = mbedtls_net_connect(&server_fd, host, port, MBEDTLS_NET_PROTO_TCP);
	if (ret < 0)
		return OTT_ERROR;

	/* Set up the SSL context */
	ret = mbedtls_ssl_setup(&ssl, &conf);
	if (ret != 0)
		return OTT_ERROR;

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send,
			mbedtls_net_recv, NULL);

	/* Perform TLS handshake */
	ret = mbedtls_ssl_handshake(&ssl);
	/* XXX: Keep timeout here? */
	uint32_t start = HAL_GetTick();
	while (ret != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			return OTT_ERROR;
		}
		if (HAL_GetTick() - start > TIMEOUT_MS) {
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
	if (mbedtls_ssl_close_notify(&ssl) == 0)
		return OTT_OK;
	mbedtls_ssl_session_reset(&ssl);
	return OTT_ERROR;
}

static bool write_tls(const uint8_t *buf, uint16_t len)
{
	/* Attempt to write 'len' bytes of 'buf' over the TCP/TLS stream. */
	int ret = mbedtls_ssl_write(&ssl, (const unsigned char *)buf, (size_t)len);
	/* XXX: Keep timeout here? */
	uint32_t start = HAL_GetTick();
	while (ret <= 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			return false;
		}
		if (HAL_GetTick() - start > TIMEOUT_MS) {
			mbedtls_ssl_session_reset(&ssl);
			return OTT_ERROR;
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
	 * Following are invalid flag settings:
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

ott_status ott_send_auth_to_cloud(c_flags_t c_flags, const uint8_t *dev_id,
				  uint16_t dev_sec_sz, const uint8_t *dev_sec)
{
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags) || OTT_FLAG_IS_SET(c_flags, CF_QUIT) ||
			(dev_sec_sz + OTT_UUID_SZ > OTT_DATA_SZ) ||
			dev_id == NULL || dev_sec == NULL || dev_sec_sz == 0)
		return OTT_INV_PARAM;

	uint16_t len = OTT_VER_SZ + OTT_OVERHEAD_SZ + OTT_UUID_SZ + dev_sec_sz;
	unsigned char buf[len];

	uint16_t idx = 0;
	/* The version byte is sent before the very first message. */
	buf[idx++] = VERSION_BYTE;

	/* Build the rest of the message. */
	buf[idx++] = (uint8_t)(c_flags << 4 | MT_AUTH);
	memcpy(buf + idx, dev_id, OTT_UUID_SZ);
	idx += OTT_UUID_SZ;
	buf[idx++] = (uint8_t)(dev_sec_sz & 0xFF);
	buf[idx++] = (uint8_t)((dev_sec_sz >> 8) & 0xFF);
	memcpy(buf + idx, dev_sec, dev_sec_sz);

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
	if (!flags_are_valid(c_flags) || OTT_FLAG_IS_SET(c_flags, CF_QUIT) ||
			(status_sz > OTT_DATA_SZ) || (status == NULL))
		return OTT_INV_PARAM;

	uint16_t len = OTT_CMD_SZ + OTT_LEN_SZ + status_sz;
	unsigned char buf[len];

	uint16_t idx = 0;
	buf[idx++] = (uint8_t)(c_flags << 4 | MT_STATUS);
	buf[idx++] = (uint8_t)(status_sz & 0xFF);
	buf[idx++] = (uint8_t)((status_sz >> 8) & 0xFF);
	memcpy(buf + idx, status, status_sz);

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
	OTT_LOAD_FLAGS(msg_buf[0], msg->c_flags);
	OTT_LOAD_MTYPE(msg_buf[0], msg->m_type);
	switch (msg->m_type) {
	case MT_UPDATE:
		msg->data->array.sz = (uint16_t)((msg_buf[2] << 8) | msg_buf[1]);
		if (msg->data->array.sz > OTT_DATA_SZ)
			return false;
		memcpy(msg->data->array.bytes, &msg_buf[3], msg->data->array.sz);
		break;
	case MT_CMD_PI:
	case MT_CMD_SL:
		msg->data->cmd_value = (uint32_t)(msg_buf[4] << 24 | msg_buf[3]
				<< 16 | msg_buf[2] << 8 | msg_buf[1]);
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

	int ret = mbedtls_ssl_read(&ssl, msg_buf, OTT_MAX_MSG_SZ);
	if (ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
			ret == MBEDTLS_ERR_SSL_WANT_READ)
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

	if (ret > 0) {	/* XXX: Assuming entire message arrives at once */
		if (!populate_msg_struct(msg)) {
			ott_send_ctrl_msg(CF_NACK | CF_QUIT);
			return OTT_NO_MSG;
		}
		return OTT_OK;
	}

	if (ret == 0)			/* No more messages to receive */
		return OTT_NO_MSG;

	return OTT_ERROR;
}

/* Stubs for now. These will be implemented by the AT layer. */
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
