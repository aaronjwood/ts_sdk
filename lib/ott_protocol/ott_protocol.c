/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>

#include "dbg.h"
#include "platform.h"
#include "ott_protocol.h"

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

/* Certificate that is used with the OTT services */
#include "verizon_ott_ca.h"

#ifdef MBEDTLS_DEBUG_C
static void my_debug(void *ctx, int level,
                     const char *file, int line,
		     const char *str)
{
	(void)(level);
	dbg_printf("%s:%04d: %s", file, line, str);
	fflush(stdout);
}
#endif

/*
 * Assumption: Underlying transport protocol is stream oriented. So parts of
 * the message can be sent through separate write calls.
 */
#define WRITE_AND_RETURN_ON_ERROR(buf, len, ret) \
	do { \
		ret = write_tls((buf), (len)); \
		if (ret == OTT_ERROR || ret == OTT_TIMEOUT) \
		return ret; \
	} while(0)

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

#ifdef BUILD_TARGET_OSX
void ott_protocol_deinit(void)
{
	/* "server_fd" and "ssl" are freed by ott_close_connection() */
	mbedtls_x509_crt_free(&cacert);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
}
#endif

ott_status ott_protocol_init(void)
{
	int ret;

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
	if (ret != 0) {
		mbedtls_net_free(&server_fd);
		return OTT_ERROR;
	}

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send,
			mbedtls_net_recv, NULL);

	/* Perform TLS handshake */
	ret = mbedtls_ssl_handshake(&ssl);
	uint32_t start = platform_get_tick_ms();
	while (ret != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			mbedtls_ssl_free(&ssl);
			mbedtls_net_free(&server_fd);
			return OTT_ERROR;
		}
		if (platform_get_tick_ms() - start > TIMEOUT_MS) {
			mbedtls_ssl_session_reset(&ssl);
			mbedtls_ssl_free(&ssl);
			mbedtls_net_free(&server_fd);
			return OTT_TIMEOUT;
		}
		ret = mbedtls_ssl_handshake(&ssl);
	}

	return OTT_OK;
}

ott_status ott_close_connection(void)
{
	/* Close the connection and notify the peer. */
	int s = mbedtls_ssl_close_notify(&ssl);
	mbedtls_ssl_free(&ssl);
	mbedtls_net_free(&server_fd);
	if (s == 0)
		return OTT_OK;
	else
		return OTT_ERROR;
}

static ott_status write_tls(const uint8_t *buf, uint16_t len)
{
	/* Attempt to write 'len' bytes of 'buf' over the TCP/TLS stream. */
	int ret = mbedtls_ssl_write(&ssl, (const unsigned char *)buf, (size_t)len);
	uint32_t start = platform_get_tick_ms();
	while (ret <= 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			return OTT_ERROR;
		}
		if (platform_get_tick_ms() - start > TIMEOUT_MS) {
			mbedtls_ssl_session_reset(&ssl);
			return OTT_TIMEOUT;
		}
		ret = mbedtls_ssl_write(&ssl, (const unsigned char *)buf,
				(size_t)len);
	}
	return OTT_OK;
}

/* Return "true" if the flag settings are valid. */
static bool flags_are_valid(c_flags_t f)
{
	/*
	 * Following are invalid flag settings:
	 * NACK + ACK
	 * NACK + PENDING
	 * PENDING + QUIT
	 * NACK + ACK + PENDING
	 * NACK + PENDING + QUIT
	 * ACK + PENDING + QUIT
	 * NACK + ACK + QUIT
	 * NACK + ACK + PENDING + QUIT
	 *
	 * Accounting for the first three takes care of the rest.
	 */
	if (OTT_FLAG_IS_SET(f, CF_NACK | CF_ACK) ||
			OTT_FLAG_IS_SET(f, CF_NACK | CF_PENDING) ||
			OTT_FLAG_IS_SET(f, CF_PENDING | CF_QUIT))
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

	uint8_t bytes[2] = {VERSION_BYTE, 0};
	ott_status ret;

	/* The version byte is sent before the very first message. */
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)bytes, 1, ret);

	/* Send the command byte */
	bytes[0] = (uint8_t)(c_flags | MT_AUTH);
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)bytes, 1, ret);

	/* Send the device ID */
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)dev_id, OTT_UUID_SZ, ret);

	/* Send the device secret size in little endian format */
	bytes[0] = (uint8_t)(dev_sec_sz & 0xFF);
	bytes[1] = (uint8_t)((dev_sec_sz >> 8) & 0xFF);
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)bytes, sizeof(uint16_t), ret);

	/* Send the device secret */
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)dev_sec, dev_sec_sz, ret);

	return OTT_OK;
}

ott_status ott_send_status_to_cloud(c_flags_t c_flags,
                                    uint16_t status_sz,
				    const uint8_t *status)
{
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags) || OTT_FLAG_IS_SET(c_flags, CF_QUIT) ||
			(status_sz > OTT_DATA_SZ) || (status == NULL))
		return OTT_INV_PARAM;

	ott_status ret;
	uint8_t bytes[2];

	/* Send the command byte */
	bytes[0] = (uint8_t)(c_flags | MT_STATUS);
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)bytes, 1, ret);

	/* Send the status length field in little endian format */
	bytes[0] = (uint8_t)(status_sz & 0xFF);
	bytes[1] = (uint8_t)((status_sz >> 8) & 0xFF);
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)bytes, sizeof(uint16_t), ret);

	/* Send the actual status data */
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)status, status_sz, ret);

	return OTT_OK;
}

ott_status ott_send_ctrl_msg(c_flags_t c_flags)
{
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags))
		return OTT_INV_PARAM;

	ott_status ret;
	unsigned char byte = (uint8_t)(c_flags | MT_NONE);

	/* Send the command byte */
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)&byte, 1, ret);

	return OTT_OK;
}

ott_status ott_send_restarted(c_flags_t c_flags)
{
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags))
		return OTT_INV_PARAM;

	ott_status ret;
	unsigned char byte = (uint8_t)(c_flags | MT_RESTARTED);

	/* Send the command byte */
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)&byte, 1, ret);

	return OTT_OK;
}

/* Return "false" if the message has invalid data, else return "true". */
static bool msg_is_valid(msg_t *msg)
{
	c_flags_t c_flags;
	OTT_LOAD_FLAGS(msg->cmd_byte, c_flags);
	if (!flags_are_valid(c_flags))
		return false;

	m_type_t m_type;
	OTT_LOAD_MTYPE(msg->cmd_byte, m_type);
	switch (m_type) {
	case MT_UPDATE:
		if (msg->data.array.sz > OTT_DATA_SZ)
			return false;
		else
			return true;
	case MT_CMD_PI:
	case MT_CMD_SL:
		/* XXX: Limit the intervals to some realistic value: 2 days? */
		return true;
	case MT_NONE:
		/* XXX: Perform additional checks? */
		return true;
	default:
		return false;
	}
}

#define UPD_OVR_HEAD		OTT_OVERHEAD_SZ
#define MIN_UPD_SIZE		(UPD_OVR_HEAD + 1 /* Actual data */)
#define MIN_CMD_PI_SIZE		(OTT_CMD_SZ + 4 /* uint32_t */)
#define MIN_CMD_SL_SIZE		(OTT_CMD_SZ + 4 /* uint32_t */)
#define MIN_MT_NONE_SIZE	OTT_CMD_SZ

/* Return true if a complete message has been received */
static bool msg_is_complete(msg_t *msg, uint16_t recvd)
{
	m_type_t m_type;
	OTT_LOAD_MTYPE(msg->cmd_byte, m_type);

	if (m_type == MT_NONE)
		return (recvd == MIN_MT_NONE_SIZE);
	if (m_type == MT_CMD_PI)
		return (recvd == MIN_CMD_PI_SIZE);
	if (m_type == MT_CMD_SL)
		return (recvd == MIN_CMD_SL_SIZE);
	if (m_type == MT_UPDATE) {
		if (recvd < MIN_UPD_SIZE)
			return false;
		return (recvd - UPD_OVR_HEAD == msg->data.array.sz);
	}

	return false;
}

ott_status ott_retrieve_msg(msg_t *msg, uint16_t sz)
{
	if (msg == NULL || sz < 4 || sz > OTT_MAX_MSG_SZ)
		return OTT_INV_PARAM;

	static uint16_t recvd = 0;	/* Number of bytes received so far */
	int ret = mbedtls_ssl_read(&ssl, (unsigned char *)msg + recvd, sz - recvd);
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

	if (ret > 0) {
		recvd += ret;
		if (msg_is_complete(msg, recvd)) {
			recvd = 0;
			if (!msg_is_valid(msg)) {
				ott_send_ctrl_msg(CF_NACK | CF_QUIT);
				ott_close_connection();
				return OTT_ERROR;
			}
			return OTT_OK;
		}
		return OTT_NO_MSG;
	}

	if (ret == 0)            /* EOF: No more messages to receive */
		return OTT_OK;

	return OTT_ERROR;
}

/* Debug functions follow */

static inline void set_tab_level(uint8_t n)
{
	for (uint8_t i = 0; i < n; i++)
		dbg_printf("\t");
}

static void interpret_type_flags(m_type_t m_type, c_flags_t c_flags, uint8_t t)
{
	set_tab_level(t);
	dbg_printf("Message type: ");
	if (m_type == MT_NONE)
		dbg_printf("MT_NONE\n");
	else if (m_type == MT_AUTH)
		dbg_printf("MT_AUTH\n");
	else if (m_type == MT_STATUS)
		dbg_printf("MT_STATUS\n");
	else if (m_type == MT_UPDATE)
		dbg_printf("MT_UPDATE\n");
	else if (m_type == MT_RESTARTED)
		dbg_printf("MT_RESTARTED\n");
	else if (m_type == MT_CMD_PI)
		dbg_printf("MT_CMD_PI\n");
	else if (m_type == MT_CMD_SL)
		dbg_printf("MT_CMD_SL\n");
	else
		dbg_printf("Invalid message type\n");

	set_tab_level(t);
	dbg_printf("Flags set: ");
	if (OTT_FLAG_IS_SET(c_flags, CF_NONE))
		dbg_printf("CF_NONE ");
	if (OTT_FLAG_IS_SET(c_flags, CF_NACK))
		dbg_printf("CF_NACK ");
	if (OTT_FLAG_IS_SET(c_flags, CF_ACK))
		dbg_printf("CF_ACK ");
	if (OTT_FLAG_IS_SET(c_flags, CF_PENDING))
		dbg_printf("CF_PENDING ");
	if (OTT_FLAG_IS_SET(c_flags, CF_QUIT))
		dbg_printf("CF_QUIT ");
	dbg_printf("\n");
}

void ott_interpret_msg(msg_t *msg, uint8_t t)
{
	if (!msg)
		return;
	m_type_t m_type;
	c_flags_t c_flags;
	uint32_t sl_int_sec;

	OTT_LOAD_MTYPE(msg->cmd_byte, m_type);
	OTT_LOAD_FLAGS(msg->cmd_byte, c_flags);
	interpret_type_flags(m_type, c_flags, t);
	switch (m_type) {
	case MT_UPDATE:
		set_tab_level(t);
		dbg_printf("Size : %"PRIu16"\n", msg->data.array.sz);
		set_tab_level(t);
		dbg_printf("Data :\n");
		for (uint8_t i = 0; i < msg->data.array.sz; i++) {
			set_tab_level(t + 1);
			dbg_printf("0x%02x\n", msg->data.array.bytes[i]);
		}
		return;
	case MT_CMD_SL:
		sl_int_sec = msg->data.interval;
		set_tab_level(t);
		dbg_printf("Sleep interval (secs): %"PRIu32"\n", sl_int_sec);
		return;
	default:
		return;
	}
}
