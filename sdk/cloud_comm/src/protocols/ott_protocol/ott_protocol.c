/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdlib.h>
#include "dbg.h"
#include "sys.h"
#include "service_ids.h"
#include "ott_protocol.h"
#include "ott_def.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

#ifdef MBEDTLS_DEBUG_C
#include "mbedtls/debug.h"
#endif

/* Sanity checks.  The cloud is primarily responsible for enforcing limits. */
/* XXX Need to set these values to allow whatever limits get documented. */
#define MIN_SANE_POLLING_INTERVAL_MS	10000
#define MAX_SANE_POLLING_INTERVAL_MS (2 * 24 * 60 * 60 * 1000) /* 2 days */

#define INVOKE_SEND_CALLBACK(_buf, _sz, _evt)	\
	do { \
		if (session.send_cb) \
			session.send_cb((_buf), (_sz), (_evt), \
					session.send_svc_id ); \
	} while(0)

#define INVOKE_RECV_CALLBACK(_buf, _sz, _evt, _svc_id)	\
	do { \
		if (session.rcv_cb) \
			session.rcv_cb((_buf), (_sz), (_evt), (_svc_id)); \
	} while(0)

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

#ifdef CALC_TLS_OVRHD_BYTES
extern bool ovrhd_profile_flag;
extern uint32_t num_bytes_recvd;
extern uint32_t num_bytes_sent;
#define START_CALC_OVRHD_BYTES()	ovrhd_profile_flag = true
#define STOP_CALC_OVRHD_BYTES()		ovrhd_profile_flag = false
#define PRINT_OVRHD_SENT()		dbg_printf("OVS:%"PRIu32"\n", num_bytes_sent);
#define PRINT_OVRHD_RECVD()		dbg_printf("OVR:%"PRIu32"\n", num_bytes_recvd);
#else
#define START_CALC_OVRHD_BYTES()
#define STOP_CALC_OVRHD_BYTES()
#define PRINT_OVRHD_SENT()
#define PRINT_OVRHD_RECVD()
#endif	/* CALC_TLS_OVRHD_BYTES */

#ifdef PROTO_HEAP_PROFILE
#include <stdlib.h>
#include <inttypes.h>
#include "mbedtls/platform.h"
static uintptr_t max_alloc = 0;
extern int _end;			/* Defined in linker script */
static uintptr_t heap_start = (uintptr_t)&_end;
extern caddr_t _sbrk(int);		/* Implemented in libnosys */

static void *ott_calloc(size_t num, size_t size)
{
	void *p = calloc(num, size);
	if (p) {
		/* Get current top of the heap */
		uintptr_t s = (uintptr_t)_sbrk(0);
		s -= heap_start;
		max_alloc = ((max_alloc >= s) ? max_alloc : s);
	}
	return p;
}
#endif

#ifdef PROTO_TIME_PROFILE
static uint32_t proto_begin;
#endif

static uint32_t current_polling_interval = INIT_POLLING_MS;

/*
 * Assumption: Underlying transport protocol is stream oriented. So parts of
 * the message can be sent through separate write calls.
 */
#define WRITE_AND_RETURN_ON_ERROR(buf, len, ret) \
	do { \
		ret = write_tls((buf), (len)); \
		if (ret == PROTO_ERROR || ret == PROTO_TIMEOUT) \
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

static void ott_reset_state(void)
{
	session.send_buf = NULL;
	session.send_sz = 0;
	session.send_cb = NULL;
	session.send_svc_id = CC_SERVICE_BASIC;
	session.conn_done = false;
	session.auth_done = false;
	session.pend_bit = false;
	session.pend_ack = false;
	session.nack_sent = false;
}

static void ott_init_state(void)
{
	session.host[0] = 0x00;
	session.port[0] = 0x00;
	session.conn_done = false;
	session.auth_done = false;
	session.pend_bit = false;
	session.pend_ack = false;
	session.nack_sent = false;
	auth.auth_valid = false;
	auth.serv_auth_valid = false;
}

proto_result ott_protocol_init(void)
{
	ott_init_state();
	int ret;

#ifdef MBEDTLS_DEBUG_C
	mbedtls_debug_set_threshold(1);
#endif

#ifdef PROTO_HEAP_PROFILE
	mbedtls_sys_set_calloc_free(ott_calloc, free);
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
		return PROTO_ERROR;
	}

	/* Set up the TLS structures */
	ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
				MBEDTLS_SSL_TRANSPORT_STREAM,
				MBEDTLS_SSL_PRESET_DEFAULT);
	if (ret != 0) {
		cleanup_mbedtls();
		return PROTO_ERROR;
	}

	mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);
#ifdef MBEDTLS_DEBUG_C
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
#endif

	return PROTO_OK;
}

proto_result ott_set_own_auth(const uint8_t *d_id, uint32_t d_id_sz,
                        const uint8_t *d_sec, uint32_t d_sec_sz)
{
	if ((d_id == NULL) || (d_id_sz != OTT_UUID_SZ))
		return PROTO_INV_PARAM;
	if ((d_sec == NULL) || (d_sec_sz != OTT_DEV_SC_SZ))
		return PROTO_INV_PARAM;
	if ((d_sec_sz + d_id_sz) > PROTO_DATA_SZ)
		return PROTO_INV_PARAM;

	/* Store the auth information for establishing connection to the cloud */
	memcpy(auth.dev_ID, d_id, d_id_sz);
	memcpy(auth.d_sec, d_sec, d_sec_sz);
	auth.auth_valid = true;
	return PROTO_OK;
}

proto_result ott_set_remote_auth(const uint8_t *serv_cert, uint32_t cert_len)
{
	auth.serv_auth_valid = false;
	if ((serv_cert == NULL) || (cert_len == 0))
		return PROTO_INV_PARAM;

	/* Load the CA root certificate */
	int ret = mbedtls_x509_crt_parse_der(&cacert,
	                                 (const unsigned char *)serv_cert,
					 cert_len);
	if (ret < 0) {
		cleanup_mbedtls();
		return PROTO_ERROR;
	}

	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	auth.serv_auth_valid = true;
	return PROTO_OK;
}

proto_result ott_set_destination(const char *dest)
{
	if (!dest)
		return PROTO_INV_PARAM;

	char *delimiter = strchr(dest, ':');
	if (delimiter == NULL)
		return PROTO_INV_PARAM;

	ptrdiff_t hlen = delimiter - dest;
	if (hlen > MAX_HOST_LEN || hlen == 0)
		return PROTO_INV_PARAM;

	size_t plen = strlen(delimiter+1);
	if (plen > MAX_PORT_LEN || plen == 0)
		return PROTO_INV_PARAM;

	strncpy(session.host, dest, hlen);
	session.host[hlen] = '\0';
	strncpy(session.port, delimiter+1, sizeof(session.port));

	return PROTO_OK;
}

proto_result ott_set_recv_buffer_cb(void *rcv_buf, uint32_t sz,
                                proto_callback rcv_cb)
{
	if (!rcv_buf || (sz > PROTO_MAX_MSG_SZ))
		return PROTO_INV_PARAM;
	session.rcv_buf = rcv_buf;
	session.rcv_sz = sz;
	session.rcv_cb = rcv_cb;
	return PROTO_OK;
}

static proto_result ott_close_connection(void)
{
	PROTO_TIME_PROFILE_BEGIN();
	/* Close the connection and notify the peer. */
	int s = mbedtls_ssl_close_notify(&ssl);
	mbedtls_ssl_free(&ssl);
	mbedtls_net_free(&server_fd);
	PROTO_TIME_PROFILE_END("CC");

#ifdef OTT_HEAP_PROFILE
	dbg_printf("[HP:%"PRIuPTR"]\n", max_alloc);
#endif

	if (s == 0)
		return PROTO_OK;
	else
		return PROTO_ERROR;
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

static proto_result write_tls(const uint8_t *buf, uint16_t len)
{
	/* Attempt to write 'len' bytes of 'buf' over the TCP/TLS stream. */
	uint64_t start = sys_get_tick_ms();
	uint16_t nbytes = 0;

	do {
		int ret = mbedtls_ssl_write(&ssl, (const unsigned char *)buf +
				nbytes, (size_t)(len - nbytes));
		if (ret < 0 && ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			return PROTO_ERROR;
		}

		if (sys_get_tick_ms() - start >= TIMEOUT_MS) {
			mbedtls_ssl_session_reset(&ssl);
			return PROTO_TIMEOUT;
		}

		if (ret > 0)
			nbytes += ret;
	} while (nbytes < len);
	return PROTO_OK;
}

static proto_result ott_send_ctrl_msg(c_flags_t c_flags)
{
	PROTO_TIME_PROFILE_BEGIN();
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags))
		return PROTO_INV_PARAM;

	proto_result ret;
	unsigned char byte = (uint8_t)(c_flags | MT_NONE);

	/* Send the command byte */
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)&byte, 1, ret);

	PROTO_TIME_PROFILE_END("SC");
	return PROTO_OK;
}

void ott_initiate_quit(bool send_nack)
{
	if (!session.conn_done || !session.auth_done)
		return;
	c_flags_t c_flags = send_nack ? (CF_NACK | CF_QUIT) : CF_QUIT;
	ott_send_ctrl_msg(c_flags);
	ott_close_connection();
        ott_reset_state();
}

static void fake_receiving_service_msg(m_type_t m_type, msg_t *msg_ptr,
				       uint32_t *rcvd_len_ptr)
{
	/*
	 * Overwrite the original payload (i.e. the interval value for the
	 * command) with a Control service protocol message so we can
	 * later dispatch the message to the Control service.
	 */
	uint32_t interval = msg_ptr->data.interval;
	uint8_t *data = (uint8_t *)msg_ptr;

	/* Payload should always start here no matter which service. */
	data += PROTO_OVERHEAD_SZ;

	switch (m_type) {
	case MT_CMD_PI:
		*data++ = CONTROL_PROTOCOL_VERSION;
		*data++ = CTRL_MSG_SET_POLLING_INTERVAL;
		memcpy(data, &interval, sizeof(uint32_t));
		*rcvd_len_ptr = *rcvd_len_ptr + 1;
		break;
	case MT_CMD_SL:
		*data++ = CONTROL_PROTOCOL_VERSION;
		*data++ = CTRL_MSG_MAKE_DEVICE_SLEEP;
		memcpy(data, &interval, sizeof(uint32_t));
		*rcvd_len_ptr = *rcvd_len_ptr + 1;
		break;
	default:
		dbg_printf("%s:%d: Unexpected OTT message type: %d\n",
			   __func__, __LINE__, m_type);
	}
}

/*
 * Process a received response. Most of the actual processing is done through
 * the user provided callbacks this function invokes. In addition, it sets some
 * internal flags to decide the state of the session in the future. Calling the
 * send callback is optional and is decided through "invoke_send_cb".
 * On receiving a NACK return "false". Otherwise, return "true".
 */
static bool process_recvd_msg(msg_t *msg_ptr, uint32_t rcvd, bool invoke_send_cb)
{
	c_flags_t c_flags;
	m_type_t m_type;
	bool no_nack_detected = true;
	proto_service_id svc_id = CC_SERVICE_BASIC;
	OTT_LOAD_FLAGS(msg_ptr->cmd_byte, c_flags);
	OTT_LOAD_MTYPE(msg_ptr->cmd_byte, m_type);

	/* Keep the session alive if the cloud has more messages to send */
	session.pend_bit = OTT_FLAG_IS_SET(c_flags, CF_PENDING);

	if (OTT_FLAG_IS_SET(c_flags, CF_ACK)) {

		/* ACK while authenticating completed authentication. */
		if (!session.auth_done)
			session.auth_done = true;

		proto_event evt = PROTO_RCVD_NONE;
		/* Messages with a body need to be ACKed in the future */
		session.pend_ack = false;

		if (m_type == MT_UPDATE || m_type == MT_CMD_SL ||
		    m_type == MT_CMD_PI)
			/* Received a message, not just an ACK */
			evt = PROTO_RCVD_MSG;

		if (evt == PROTO_RCVD_MSG) {
			/* XXX OTT doesn't have service id's yet, so fake it. */
			if (m_type == MT_CMD_SL || m_type == MT_CMD_PI) {
				fake_receiving_service_msg(m_type, msg_ptr,
							   &rcvd);
				svc_id = CC_SERVICE_CONTROL;
			}
			INVOKE_RECV_CALLBACK(msg_ptr, rcvd, evt, svc_id);
		}

		if (invoke_send_cb) {
			INVOKE_SEND_CALLBACK(session.send_buf, session.send_sz,
						PROTO_RCVD_ACK);
		}
	} else if (OTT_FLAG_IS_SET(c_flags, CF_NACK)) {
		no_nack_detected = false;
		if (invoke_send_cb) {
			INVOKE_SEND_CALLBACK(session.send_buf, session.send_sz,
				PROTO_RCVD_NACK);
		}
	}

	if (OTT_FLAG_IS_SET(c_flags, CF_QUIT)) {
		ott_close_connection();
		ott_reset_state();
		INVOKE_RECV_CALLBACK(msg_ptr, rcvd, PROTO_RCVD_QUIT, svc_id);
	}

	return no_nack_detected;
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
		if (msg->data.array.sz > PROTO_DATA_SZ)
			return false;
		else
			return true;
	case MT_CMD_PI:
	case MT_CMD_SL:
	case MT_NONE:
		/* XXX: Perform additional checks? */
		return true;
	default:
		return false;
	}
}

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

static proto_result ott_retrieve_msg(msg_t *msg, uint16_t sz, uint32_t *rbytes)
{
	if (msg == NULL || sz < 4 || sz > PROTO_MAX_MSG_SZ)
		return PROTO_INV_PARAM;

	static uint32_t recvd = 0;	/* Number of bytes received so far */
	int ret = mbedtls_ssl_read(&ssl, (unsigned char *)msg + recvd, sz - recvd);
	if (ret == MBEDTLS_ERR_SSL_WANT_WRITE ||
			ret == MBEDTLS_ERR_SSL_WANT_READ)
		return PROTO_NO_MSG;

	if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
		if (ott_close_connection() != PROTO_OK)
			return PROTO_ERROR;
		return PROTO_NO_MSG;
	}

	if (ret < 0) {
		mbedtls_ssl_session_reset(&ssl);
		return PROTO_ERROR;
	}

	if (ret > 0) {
		recvd += ret;
		if (msg_is_complete(msg, recvd)) {
			*rbytes = recvd;
			recvd = 0;
			if (!msg_is_valid(msg)) {
				ott_send_ctrl_msg(CF_NACK | CF_QUIT);
				ott_close_connection();
				return PROTO_ERROR;
			}
			return PROTO_OK;
		}
		return PROTO_NO_MSG;
	}

	if (ret == 0)            /* EOF: No more messages to receive */
		return PROTO_OK;

	return PROTO_ERROR;
}

/*
 * Attempt to receive a response within a timeout. Once a response is received,
 * process it. This helper function must be called everytime the device sends
 * a message to the cloud and expects a response in return.
 * "invoke_send_cb" decides if the send callback should be invoked on a valid
 * response. When expecting a response for an auth message (or any internal
 * message), this should be false.
 * On successfully receiving a valid response, this function will return "true".
 * On receiving a NACK or timeout, return "false".
 */
static bool recv_resp_within_timeout(uint32_t timeout, bool invoke_send_cb)
{
	PROTO_TIME_PROFILE_BEGIN();

	uint32_t start = sys_get_tick_ms();
	uint32_t end = start;
	bool no_nack = true;
	uint32_t rcvd = 0;
	do {
		proto_result s = ott_retrieve_msg(session.rcv_buf,
						session.rcv_sz, &rcvd);
		if (s == PROTO_INV_PARAM || s == PROTO_ERROR)
			return false;
		if (s == PROTO_NO_MSG) {
			end = sys_get_tick_ms();
			continue;
		}
		if (s == PROTO_OK) {
			PROTO_TIME_PROFILE_END("RV");
			no_nack = process_recvd_msg(session.rcv_buf, rcvd,
						invoke_send_cb);
			break;
		}
	} while(end - start < timeout);

	if (end - start >= timeout) {
		if (invoke_send_cb)
			INVOKE_SEND_CALLBACK(session.send_buf, session.send_sz,
					     PROTO_SEND_TIMEOUT);
		return false;
	}

	return no_nack;
}

static proto_result ott_initiate_connection(const char *host, const char *port)
{
	START_CALC_OVRHD_BYTES();
	PROTO_TIME_PROFILE_BEGIN();
	if (host == NULL || port == NULL)
		return PROTO_INV_PARAM;

	int ret;
	/* Connect to the cloud server over TCP */
	ret = mbedtls_net_connect(&server_fd, host, port,
				  MBEDTLS_NET_PROTO_TCP);
	if (ret < 0) {
		mbedtls_net_free(&server_fd);
		return PROTO_ERROR;
	}

	/* Set up the SSL context */
	ret = mbedtls_ssl_setup(&ssl, &conf);
	if (ret != 0) {
		mbedtls_net_free(&server_fd);
		return PROTO_ERROR;
	}

	mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send,
			mbedtls_net_recv, NULL);

	/*
	 * Set the server identity (hostname) that must be present in its
	 * certificate CN or SubjectAltName.
	 */
	dbg_printf("\tSetting required server identity\n");
	ret = mbedtls_ssl_set_hostname(&ssl, host);
	if (ret != 0)
		return PROTO_ERROR;

	/* Perform TLS handshake */
	ret = mbedtls_ssl_handshake(&ssl);
	uint32_t start = sys_get_tick_ms();
	while (ret != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			mbedtls_ssl_free(&ssl);
			mbedtls_net_free(&server_fd);
			return PROTO_ERROR;
		}
		if (sys_get_tick_ms() - start > TIMEOUT_MS) {
			mbedtls_ssl_session_reset(&ssl);
			mbedtls_ssl_free(&ssl);
			mbedtls_net_free(&server_fd);
			return PROTO_TIMEOUT;
		}
		ret = mbedtls_ssl_handshake(&ssl);
	}

	PROTO_TIME_PROFILE_END("IC");
	STOP_CALC_OVRHD_BYTES();
	PRINT_OVRHD_SENT();
	PRINT_OVRHD_RECVD();
	return PROTO_OK;
}

/*
 * Send the authentication message to the cloud service. Transactions in the TLS
 * session must begin only after a successful call to this API (i.e. it exits
 * with PROTO_OK) and after checking if the ACK flag is set in the received
 * response. An auth message can be sent only after a call to
 * ott_initiate_connection(). This call is blocking in nature.
 *
 */
static proto_result ott_send_auth_to_cloud(c_flags_t c_flags)
{
	if (!auth.auth_valid && !auth.serv_auth_valid)
		return PROTO_ERROR;

	const uint8_t *dev_id = auth.dev_ID;
	uint32_t dev_sec_sz = OTT_DEV_SC_SZ;
	const uint8_t *dev_sec = auth.d_sec;

	PROTO_TIME_PROFILE_BEGIN();
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags) || OTT_FLAG_IS_SET(c_flags, CF_QUIT))
		return PROTO_ERROR;

	uint8_t bytes[2] = {VERSION_BYTE, 0};
	proto_result ret;

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

	PROTO_TIME_PROFILE_END("SA");
	return PROTO_OK;
}

static bool establish_session(bool polling)
{
	if (strlen(session.host) == 0 || strlen(session.port) == 0)
		return false;
	if (!session.rcv_buf)
		return false;

retry_connection:
	if (ott_initiate_connection(session.host, session.port) != PROTO_OK)
		return false;
	session.conn_done = true;
	/* Send the authentication message to the cloud. If this is a call to
	 * simply poll the cloud for possible messages, do not set the PENDING
	 * flag.
	 */
	c_flags_t c_flags = polling ? CF_NONE : CF_PENDING;
	if (ott_send_auth_to_cloud(c_flags) != PROTO_OK) {
		ott_initiate_quit(false);
		return false;
	}

	/* This call should not invoke the send callback */
	if (!recv_resp_within_timeout(RECV_TIMEOUT_MS, false)) {
		ott_initiate_quit(true);
		return false;
	}

	/* If we NACKed an incoming message, the session was ended. Retry. */
	if (session.nack_sent) {
		session.nack_sent = false;
		goto retry_connection;
	}

	/* If neither side has anything to send while polling, the connection
	 * would have been terminated in recv_resp_within_timeout().
	 */
	if (polling && !session.conn_done) {
		session.auth_done = false;
		return false;
	}
	return true;
}

/*
 * Send a status message to the cloud service. The message is successfully
 * delivered if this call exits with PROTO_OK and the received response has the
 * ACK flag set.
 * This call is blocking in nature.
 *
 * Parameters:
 *	c_flags   : Control flags
 *	status_sz : Size of the status data in bytes.
 * 	status    : Pointer to binary data that stores the status to be reported.
 *
 * Returns:
 * 	PROTO_OK        : Status report was sent to the cloud.
 * 	PROTO_INV_PARAM : Flag parameter is invalid (Eg. ACK+NACK) or
 * 	                  status has length > MAX_DATA_SZ bytes or
 * 	                  status is NULL
 * 	PROTO_ERROR     : Sending the message failed due to a TCP/TLS error.
 * 	PROTO_TIMEOUT   : Timed out sending the message. Sending failed.
 */
static proto_result ott_send_status_to_cloud(c_flags_t c_flags,
                                    uint16_t status_sz,
				    const uint8_t *status)
{
	PROTO_TIME_PROFILE_BEGIN();
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags) || OTT_FLAG_IS_SET(c_flags, CF_QUIT) ||
			(status_sz > PROTO_DATA_SZ) || (status == NULL))
		return PROTO_INV_PARAM;

	proto_result ret;
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

	PROTO_TIME_PROFILE_END("SS");
	return PROTO_OK;
}

/*
 * Until OTT supports service ID's we can only support operations
 * that map onto the existing protocol.  That limits us to the Control service,
 * which has only one upstream operation for OTT.
 */
static proto_result fake_sending_service_msg(const void* buf, uint32_t sz,
					     proto_service_id svc_id,
					     proto_callback cb)
{
	if (svc_id != CC_SERVICE_CONTROL)
		return PROTO_INV_PARAM;

	if (sz > 0 && *((uint8_t *)(buf) + 1) == CTRL_MSG_REQUEST_RESEND_INIT) {
		session.send_svc_id = CC_SERVICE_CONTROL;
		return ott_resend_init_config(cb);
	} else
		return PROTO_INV_PARAM;
}


proto_result ott_send_msg_to_cloud(const void *buf, uint32_t sz,
				   proto_service_id svc_id, proto_callback cb)
{
	if (!buf || sz == 0)
		return PROTO_INV_PARAM;

	/* XXX Hack until OTT is redefined to support service IDs */
	if (svc_id != CC_SERVICE_BASIC)
		return fake_sending_service_msg(buf, sz, svc_id, cb);

	if (strlen(session.host) == 0 || strlen(session.port) == 0)
		return PROTO_INV_PARAM;

	/*
	 * If a session hasn't been established, initiate a connection. This
	 * leads to the device being authenticated.
	 */
	if (!session.auth_done)
		if (!establish_session(false))
			return PROTO_ERROR;

	/*
	 * If the user ACKed a previous message, set the flag in this message.
	 * Also, make sure the PENDING flag is always set so that the device has
	 * full control on when to disconnect.
	 */
	c_flags_t c_flags = session.pend_ack ? (CF_PENDING | CF_ACK) :
		CF_PENDING;

	proto_result res = ott_send_status_to_cloud(c_flags, sz, buf);
	if (res != PROTO_OK) {
		ott_initiate_quit(false);
		return res;
	}
	session.send_buf = buf;
	session.send_sz = sz;
	session.send_cb = cb;
	session.send_svc_id = svc_id;
	session.pend_ack = false;

	/* Receive a message within a timeout and invoke the send callback */
	if (!recv_resp_within_timeout(RECV_TIMEOUT_MS, true)) {
		ott_initiate_quit(true);
		return PROTO_ERROR;
	}

	if (session.nack_sent)
		session.nack_sent = false;

	return PROTO_OK;
}

const uint8_t *ott_get_rcv_buffer_ptr(const void *msg)
{
	if (!msg)
		return NULL;
	return (uint8_t *)msg + PROTO_OVERHEAD_SZ;
}

void ott_send_ack(void)
{
        session.pend_ack = true;
}

void ott_send_nack(void)
{
        ott_initiate_quit(true);
        session.nack_sent = true;
}

/* XXX Remove this when we no longer need to fake Control service messages. */
static proto_result ott_send_restarted(c_flags_t c_flags)
{
	PROTO_TIME_PROFILE_BEGIN();
	/* Check for correct parameters */
	if (!flags_are_valid(c_flags))
		return PROTO_INV_PARAM;

	proto_result ret;
	unsigned char byte = (uint8_t)(c_flags | MT_RESTARTED);

	/* Send the command byte */
	WRITE_AND_RETURN_ON_ERROR((unsigned char *)&byte, 1, ret);

	PROTO_TIME_PROFILE_END("SR");
	return PROTO_OK;
}

/* XXX Remove this when we no longer need to fake Control service messages. */
proto_result ott_resend_init_config(proto_callback cb)
{
	if (strlen(session.host) == 0 || strlen(session.port) == 0)
		return PROTO_ERROR;

	/*
	 * If a session hasn't been established, initiate a connection. This
	 * leads to the device being authenticated.
	 */
	if (!session.auth_done)
		if (!establish_session(false))
			return PROTO_ERROR;

	/*
	 * If the user ACKed a previous message, set the flag in this message.
	 * Also, make sure the PENDING flag is always set so that the device has
	 * full control on when to disconnect.
	 */
	c_flags_t c_flags = session.pend_ack ? (CF_PENDING | CF_ACK) :
		CF_PENDING;
	if (ott_send_restarted(c_flags) != PROTO_OK) {
		ott_initiate_quit(false);
		return PROTO_ERROR;
	}

	session.pend_ack = false;
	session.send_cb = cb;

	/* Receive a message within a timeout and invoke the send callback */
	if (!recv_resp_within_timeout(RECV_TIMEOUT_MS, true)) {
		ott_initiate_quit(true);
		return PROTO_ERROR;
	}

	if (session.nack_sent)
		session.nack_sent = false;

	return PROTO_OK;
}

uint32_t ott_get_polling_interval(void)
{
        return current_polling_interval;
}

void ott_set_polling_interval(uint32_t interval_ms)
{
	if (interval_ms < MIN_SANE_POLLING_INTERVAL_MS ||
	    interval_ms > MAX_SANE_POLLING_INTERVAL_MS) {
		dbg_printf("Invalid polling interval: %"PRIu32" ms\n",
			   interval_ms);
	} else {
		dbg_printf("Setting polling interval to: %"PRIu32" ms\n",
			   interval_ms);
		current_polling_interval = interval_ms;
	}
}

/*
 * Receive any pending messages from the cloud or ACK any previous message. If
 * the message was to be NACKed, it would have been already done through the
 * receive callback.
 */
void ott_maintenance(bool poll_due)
{
	if (session.auth_done || poll_due) {
		if (!session.auth_done)
			if (!establish_session(true))
				return;
		while (session.pend_bit || session.pend_ack) {
			c_flags_t c_flags = session.pend_ack ? CF_ACK : CF_NONE;
			c_flags |= ((!session.pend_bit) ? CF_QUIT : CF_NONE);
			ott_send_ctrl_msg(c_flags);
			if (!session.pend_bit) { /* If last message in session */
				ott_close_connection();
				ott_reset_state();
				return;
			}

			if (!recv_resp_within_timeout(RECV_TIMEOUT_MS, true)) {
				ott_initiate_quit(true);
				return;
			}

			if (session.nack_sent)
				session.nack_sent = false;
		}
	}
}
