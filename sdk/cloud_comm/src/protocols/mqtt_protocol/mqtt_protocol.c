/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdio.h>
#include <string.h>

#include "mqtt_def.h"
#include "mqtt_protocol.h"
#include "MQTTClient.h"

#include "sys.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/pk.h"

#ifdef MBEDTLS_DEBUG_C
#include "dbg.h"
#include "mbedtls/debug.h"
#endif

#define RETURN_ERROR(string, ret) \
	do { \
		PRINTF_ERR("%s:%d:" #string, __func__, __LINE__); \
		PRINTF_ERR("\n"); \
		return (ret); \
	} while (0)


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

#define TIMEOUT_MS			5000

#ifdef CALC_TLS_OVRHD_BYTES
#include "dbg.h"
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

/* mbedTLS specific variables */
static mbedtls_net_context ctx;
static mbedtls_entropy_context entropy;
static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_ssl_context ssl;
static mbedtls_ssl_config conf;
static mbedtls_x509_crt cacert;
static mbedtls_x509_crt cl_cert;
static mbedtls_pk_context cl_key;

/* seed for random number used in mbedtls lib */
static const char pers[] = "mqtt_ts_sdk";

/* Intermediate buffers required by mqtt paho library */
static uint8_t send_intr_buf[MQTT_SEND_SZ];
static uint8_t recv_intr_buf[MQTT_RCV_SZ];

static char device_id[MQTT_DEVICE_ID_SZ];
static char sub_command[MQTT_TOPIC_SZ];
static char pub_command_rsp[MQTT_TOPIC_SZ];
static char pub_unit_on_board[MQTT_TOPIC_SZ];

static void cleanup_mbedtls(void)
{
	/* Free network interface resources. */
	mbedtls_net_free(&ctx);
	mbedtls_x509_crt_free(&cacert);
	mbedtls_x509_crt_free(&cl_cert);
	mbedtls_pk_free(&cl_key);
	mbedtls_ssl_free(&ssl);
	mbedtls_ssl_config_free(&conf);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_entropy_free(&entropy);
}

void TimerInit(Timer *timer)
{
	timer->end_time = 0;
}

char TimerIsExpired(Timer *timer)
{
	uint64_t now = sys_get_tick_ms();
	return timer->end_time <= now;
}

void TimerCountdownMS(Timer *timer, unsigned int ms)
{
	uint64_t now = sys_get_tick_ms();
	timer->end_time = now + ms;
}

void TimerCountdown(Timer *timer, unsigned int sec)
{
	uint64_t now = sys_get_tick_ms();
	timer->end_time = now + sec * 1000;
}

int TimerLeftMS(Timer *timer)
{
	uint64_t now = sys_get_tick_ms();
	return timer->end_time <= now ? 0 : (int)(timer->end_time - now);
}

/*
 * Attempt to read 'len' bytes from the TCP/TLS stream into buffer 'b' within
 * 'timeout_ms' ms.
 */
static int read_fn(Network *n, unsigned char *b, int len, int timeout_ms)
{
	uint64_t start_time = sys_get_tick_ms();
	int nbytes = 0;
	do {
		int recvd = mbedtls_ssl_read(&ssl, b + nbytes, len - nbytes);
		if (recvd == 0)
			return 0;
		if (recvd < 0 && recvd != MBEDTLS_ERR_SSL_WANT_READ &&
				recvd != MBEDTLS_ERR_SSL_WANT_WRITE)
			return -1;
		if (recvd > 0)
			nbytes += recvd;
	} while (nbytes < len && sys_get_tick_ms() - start_time < timeout_ms);
	return nbytes;
}

/*
 * Attempt writing to TCP/TLS stream a buffer 'b' of length 'len' within
 * 'timeout_ms' ms. Return -1 for an error, a positive number for the number of
 * bytes actually written.
 */
static int write_fn(Network *n, unsigned char *b, int len, int timeout_ms)
{
	uint64_t start_time = sys_get_tick_ms();
	int nbytes = 0;
	do {
		int ret = mbedtls_ssl_write(&ssl, b + nbytes, len - nbytes);
		if (ret < 0 && ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			return -1;
		}
		if (ret >= 0)
			nbytes += ret;
	} while (nbytes < len && sys_get_tick_ms() - start_time < timeout_ms);
	return nbytes;
}

static bool init_certs(const auth_creds *creds)
{
	/* Load the CA root certificate */
	if (mbedtls_x509_crt_parse_der(&cacert, creds->serv_cert,
					 creds->serv_cert_len) < 0) {
		cleanup_mbedtls();
		return false;
	}

	/* Load the client certificate */
	if (mbedtls_x509_crt_parse_der(&cl_cert, creds->cl_cert,
					 creds->cl_cert_len) < 0) {
		cleanup_mbedtls();
		return false;
	}

	/* Load the client key */
	if (mbedtls_pk_parse_key(&cl_key, creds->cl_key, creds->cl_key_len,
			NULL, 0) != 0) {
		cleanup_mbedtls();
		return false;
	}

	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	if (mbedtls_ssl_conf_own_cert(&conf, &cl_cert, &cl_key) != 0) {
		cleanup_mbedtls();
		return false;
	}
	return true;
}

static void mqtt_reset_state(void)
{
	session.send_buf = NULL;
	session.send_sz = 0;
	session.send_cb = NULL;
	session.send_svc_id = CC_SERVICE_BASIC;
	session.conn_valid = false;
	session.auth_valid = false;
        session.host_valid = false;
}

static void mqtt_init_state(void)
{
	session.host = NULL;
	session.port = NULL;
        session.rcv_buf = NULL;
        session.rcv_sz = 0;
        mqtt_reset_state();
}

proto_result mqtt_protocol_init(void)
{
	mqtt_init_state();
	int ret;

#ifdef MBEDTLS_DEBUG_C
	mbedtls_debug_set_threshold(1);
#endif

        mbedtls_net_init(&ctx);
        sessoin.net.mqttread = read_fn;
	session.net.mqttwrite = write_fn;
        session.mqtt_conn_data = MQTTPacket_connectData_initializer;

	/* Initialize TLS structures */
	mbedtls_ssl_init(&ssl);
	mbedtls_ssl_config_init(&conf);
	mbedtls_x509_crt_init(&cacert);
	mbedtls_x509_crt_init(&cl_cert);
	mbedtls_pk_init(&cl_key);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	/* Seed the RNG */
	mbedtls_entropy_init(&entropy);
	if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
				(const unsigned char *)pers, strlen(pers)) != 0) {
		cleanup_mbedtls();
		return false;
	}

        /* Set up the TLS structures */
        if (mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT,
                                MBEDTLS_SSL_TRANSPORT_STREAM,
                                MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
                cleanup_mbedtls();
                return false;
        }

        mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
	mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

#ifdef MBEDTLS_DEBUG_C
	mbedtls_ssl_conf_dbg(&conf, my_debug, stdout);
#endif

	return PROTO_OK;
}

static int mqtt_net_connect()
{
	int ret = 0;
	START_CALC_OVRHD_BYTES();
	/* Connect to the cloud services over TCP */
	if (mbedtls_net_connect(&ctx, session.host, session.port,
                MBEDTLS_NET_PROTO_TCP) < 0) {
		ret = -1;
		goto exit_func;
	}

	/* Set up the SSL context */
	if (mbedtls_ssl_setup(&ssl, &conf) != 0) {
		mbedtls_net_free(&ctx);
		ret = -1;
		goto exit_func;
	}

	/*
	 * Set the server identity (hostname) that must be present in its
	 * certificate CN or SubjectAltName.
	 */
	if (mbedtls_ssl_set_hostname(&ssl, session.host) != 0) {
		ret = -1;
		goto exit_func;
	}

	mbedtls_ssl_set_bio(&ssl, &ctx, mbedtls_net_send, mbedtls_net_recv, NULL);

	/* Perform TLS handshake */
	ret = mbedtls_ssl_handshake(&ssl);
	uint64_t start = sys_get_tick_ms();
	while (ret != 0) {
		if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
				ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
			mbedtls_ssl_session_reset(&ssl);
			mbedtls_ssl_free(&ssl);
			mbedtls_net_free(&ctx);
			ret = -1;
			goto exit_func;
		}
		if (sys_get_tick_ms() - start > TIMEOUT_MS) {
			mbedtls_ssl_session_reset(&ssl);
			mbedtls_ssl_free(&ssl);
			mbedtls_net_free(&ctx);
			ret = -2;
			goto exit_func;
		}
		ret = mbedtls_ssl_handshake(&ssl);
	}

exit_func:
	STOP_CALC_OVRHD_BYTES();
	PRINT_OVRHD_SENT();
	PRINT_OVRHD_RECVD();
	return ret;
}

static bool mqtt_net_disconnect(void)
{
	int s = mbedtls_ssl_close_notify(&ssl);
	mbedtls_ssl_free(&ssl);
	mbedtls_net_free(&ctx);
	if (s == 0)
		return true;
	else
		return false;
}

static void mqtt_rcvd_msg(MessageData *md)
{
	MQTTMessage *m = md->message;
	dbg_printf("%d\n", (int)m->payloadlen);
	msg.payloadlen = m->payloadlen;
	for (uint8_t i = 0; i < m->payloadlen; i++) {
		dbg_printf("%c", ((char *)m->payload)[i]);
		((char *)msg.payload)[i] = ((char *)m->payload)[i];
	}
	((char *)msg.payload)[msg.payloadlen] = '\0';
	dbg_printf("\n");
	recvd = true;
}

static bool reg_pub_sub()
{
        snprintf(sub_command, sizeof(sub_command), MQTT_SERV_PUBL_COMMAND,
		device_id);
        snprintf(pub_unit_on_board, sizeof(pub_unit_on_board),
                MQTT_PUBL_UNIT_ON_BOARD, device_id);
        if (MQTTSubscribe(&session.mclient, sub_command, MQTT_QOS_LVL,
                mqtt_rcvd_msg) < 0) {
		dbg_printf("%s:%d: MQTT Subscription failed\n",
			__func__, __LINE__);
		return false;
	}

	dbg_printf("Subscribed to topic %s successful\n", sub_command);
	return true;
}

static bool mqtt_client_and_topic_init()
{
        MQTTClientInit(&session.mclient, &session.net, TIMEOUT_MS,
                send_intr_buf, MQTT_SEND_SZ, recv_intr_buf, MQTT_RCV_SZ);

        if (!sys_get_device_id(device_id, MQTT_DEVICE_ID_SZ)) {
		dbg_printf("%s:%d: Can not retrieve device id\n",
			__func__, __LINE__);
		return false;
	}

	session.mqtt_conn_data.willFlag = MQTT_WILL;
	session.mqtt_conn_data.MQTTVersion = MQTT_PROTO_VERSION;
	session.mqtt_conn_data.clientID.cstring = device_id;
	session.mqtt_conn_data.username.cstring = NULL;
	session.mqtt_conn_data.password.cstring = NULL;
	session.mqtt_conn_data.keepAliveInterval = MQTT_KEEPALIVE_INT_SEC;
	session.mqtt_conn_data.cleansession = MQTT_CLEAN_SESSION;

        if (MQTTConnect(&session.mclient, &session.mqtt_conn_data) < 0) {
		dbg_printf("%s:%d: MQTT connect failed\n",
			__func__, __LINE__);
		return false;
	}
	dbg_printf("MQTT connect succeeded\n");
	if (!reg_pub_sub())
		return false;
	return true;
}

static bool __mqtt_net_connect(bool flag)
{
        /*
         * If a session hasn't been established, initiate a connection. This
         * indicates very first message being sent
         */
        if (!session.conn_valid && flag) {
                int ret = mqtt_net_connect();
                if (r == -1) {
        		dbg_printf("%s:%d:Error in SSL handshake\n",
                                __func__, __LINE__);
                        return false;
                }
        	if (r == -2) {
        		dbg_printf("%s:%d: SSL handshake timeout\n",
                                __func__, __LINE__);
                        return false;
                }
                if (!mqtt_client_and_topic_init())
			return false;
                session.conn_valid = true;
        }
        return true;
}

proto_result mqtt_set_auth(const auth_creds *creds)
{
	if (creds == NULL)
		RETURN_ERROR("credentials is null", PROTO_INV_PARAM);
	if ((creds->serv_cert == NULL) || (creds->cl_cert == NULL) ||
                (creds->cl_key == NULL))
		RETURN_ERROR("certs is null", PROTO_INV_PARAM);

        if (!init_certs(creds))
                RETURN_ERROR("certs initialization failed", PROTO_INV_PARAM);

	session.auth_valid = true;
        if (!__mqtt_net_connect(session.host_valid))
                RETURN_ERROR("remote connect failed", PROTO_ERROR);
	return PROTO_OK;
}

proto_result mqtt_set_destination(const char *dest)
{
	if (!dest)
		RETURN_ERROR("destination is null", PROTO_INV_PARAM);

	char *delimiter = strchr(dest, ':');
	if (delimiter == NULL)
		RETURN_ERROR("destination invalid", PROTO_INV_PARAM);

	ptrdiff_t hlen = delimiter - dest;
	if (hlen > MAX_HOST_LEN || hlen == 0)
		RETURN_ERROR("host length invalid", PROTO_INV_PARAM);

	size_t plen = strlen(delimiter + 1);
	if (plen > MAX_PORT_LEN || plen == 0)
		RETURN_ERROR("port length invalid", PROTO_INV_PARAM);

	strncpy(session.host, dest, hlen);
	session.host[hlen] = '\0';
	strncpy(session.port, delimiter + 1, plen);
        session.port[plen] = '\0';
        session.host_valid = true;

        if (!__mqtt_net_connect(session.auth_valid))
                RETURN_ERROR("remote connect failed", PROTO_ERROR);
	return PROTO_OK;
}

proto_result mqtt_set_recv_buffer_cb(void *rcv_buf, uint32_t sz,
                                proto_callback rcv_cb)
{
	if (!rcv_buf || (sz > PROTO_MAX_MSG_SZ))
		RETURN_ERROR("buffer or size invalid", PROTO_INV_PARAM);
	session.rcv_buf = rcv_buf;
	session.rcv_sz = sz;
	session.rcv_cb = rcv_cb;
	return PROTO_OK;
}

proto_result mqtt_send_msg_to_cloud(const void *buf, uint32_t sz,
				   proto_service_id svc_id, proto_callback cb)
{

	if (!buf || sz == 0)
		RETURN_ERROR("buffer or size invalid", PROTO_INV_PARAM);

	if (!session.host_valid)
		RETURN_ERROR("mqtt_set_destination needs to be called first",
                        PROTO_ERROR);
        if (!session.auth_valid)
		RETURN_ERROR("mqtt_set_auth needs to be called first",
                        PROTO_ERROR);
        if (!session.conn_valid)
                RETURN_ERROR("No active connection", PROTO_ERROR);

	session.send_buf = buf;
	session.send_sz = sz;
	session.send_cb = cb;
	session.send_svc_id = svc_id;

	return PROTO_OK;
}

const uint8_t *mqtt_get_rcv_buffer_ptr(const void *msg)
{
	if (!msg)
		return NULL;
	return (uint8_t *)msg + PROTO_OVERHEAD_SZ;
}
