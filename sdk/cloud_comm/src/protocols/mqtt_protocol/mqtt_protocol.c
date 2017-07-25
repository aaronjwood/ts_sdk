/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdio.h>
#include <string.h>

#include "mqtt_def.h"
#include "mqtt_protocol.h"
#include "paho_mqtt_port.h"
#include "MQTTClient.h"

#include "sys.h"
#include "utils.h"
#include "dbg.h"

#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "mbedtls/pk.h"

#ifdef MBEDTLS_DEBUG_C
#include "mbedtls/debug.h"
#endif

#define RETURN_ERROR(string, ret) \
	do { \
		PRINTF_ERR("%s:%d:" #string, __func__, __LINE__); \
		PRINTF_ERR("\n"); \
		return (ret); \
	} while (0)

#define IN_FUNCTION_AT() \
	do { \
		PRINTF_FUNC("%s:%d:\n", __func__, __LINE__);\
	} while (0)

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

#define TIMEOUT_MS			5000

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

/* Intermediate buffers to de-/serialize required by mqtt paho library */
static unsigned char send_intr_buf[MQTT_SEND_SZ];
static unsigned char recv_intr_buf[MQTT_RCV_SZ];

static char device_id[MQTT_DEVICE_ID_SZ];
static char sub_command[MQTT_TOPIC_SZ];
static char pub_command_rsp[MQTT_TOPIC_SZ];
static char pub_unit_on_board[MQTT_TOPIC_SZ];

static uint32_t current_polling_interval = INIT_POLLING_MS;

/* Paho mqtt related variables */
static MQTTPacket_connectData mqtt_conn_data =
					MQTTPacket_connectData_initializer;
static Network net;
static MQTTClient mclient;
static MQTTMessage msg;

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

static bool init_own_certs(const uint8_t *cli_cert, uint32_t cert_len,
			const uint8_t *cli_key, uint32_t key_len)
{
	/* Load the client certificate */
	if (mbedtls_x509_crt_parse_der(&cl_cert, cli_cert, cert_len) < 0) {
		cleanup_mbedtls();
		return false;
	}

	/* Load the client key */
	if (mbedtls_pk_parse_key(&cl_key, cli_key, key_len, NULL, 0) != 0) {
		cleanup_mbedtls();
		return false;
	}

	if (mbedtls_ssl_conf_own_cert(&conf, &cl_cert, &cl_key) != 0) {
		cleanup_mbedtls();
		return false;
	}
	return true;
}

static bool init_remote_certs(const uint8_t *serv_cert, uint32_t cert_len)
{
	/* Load the CA root certificate */
	if (mbedtls_x509_crt_parse_der(&cacert, serv_cert, cert_len) < 0) {
		cleanup_mbedtls();
		return false;
	}

	mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
	return true;
}

static void mqtt_reset_state(void)
{
	session.send_cb = NULL;
	session.send_svc_id = CC_SERVICE_BASIC;
	session.conn_valid = false;
	session.own_auth_valid = false;
	session.remote_auth_valid = false;
	session.host_valid = false;
}

static void mqtt_init_state(void)
{
        session.rcv_buf = NULL;
        session.rcv_sz = 0;
        mqtt_reset_state();
}

proto_result mqtt_protocol_init(void)
{
	mqtt_init_state();

#ifdef MBEDTLS_DEBUG_C
	mbedtls_debug_set_threshold(1);
#endif

	mbedtls_net_init(&ctx);
	net.mqttread = read_fn;
	net.mqttwrite = write_fn;

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
	if (mbedtls_net_set_nonblock(&ctx) != 0) {
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
	if (mbedtls_ssl_set_hostname(&ssl, "simpm.thingspace.verizon.com") != 0) {
		ret = -1;
		goto exit_func;
	}

	mbedtls_ssl_set_bio(&ssl, &ctx, mbedtls_net_send,
			mbedtls_net_recv, NULL);

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

static void mqtt_rcvd_msg(MessageData *md)
{
	if (!session.rcv_buf || !session.rcv_cb) {
		dbg_printf("%s:%d, mqtt_set_recv_buffer_cb needs to be called "
		"to receive new messages\n", __func__, __LINE__);
		return;
	}

	MQTTMessage *m = md->message;
	PRINTF("%s:%d: received payloadlen: %d\n",
		__func__, __LINE__, (int)m->payloadlen);
	if ((uint32_t)m->payloadlen > session.rcv_sz) {
		dbg_printf("%s: %d: buffer overflow detected\n",
				__func__, __LINE__);
		dbg_printf("%s:%d, rcvd payload len %d is greater then "
			"rcv buffer sz %u\n", __func__, __LINE__, m->payloadlen,
			session.rcv_sz);
		INVOKE_RECV_CALLBACK(session.rcv_buf, m->payloadlen,
			PROTO_RCVD_MEM_OVRFL, CC_SERVICE_BASIC);
		return;
	}
	if (strncmp(sub_command, md->topicName->lenstring.data,
		md->topicName->lenstring.len) != 0) {
		INVOKE_RECV_CALLBACK(session.rcv_buf, m->payloadlen,
			PROTO_RCVD_WRONG_MSG, CC_SERVICE_BASIC);
		return;
	}

	memcpy(session.rcv_buf, m->payload, m->payloadlen);
	INVOKE_RECV_CALLBACK(session.rcv_buf, m->payloadlen, PROTO_RCVD_MSG,
		CC_SERVICE_BASIC);
}

static bool reg_pub_sub()
{
        snprintf(sub_command, sizeof(sub_command), MQTT_SERV_PUBL_COMMAND,
		device_id);
        snprintf(pub_unit_on_board, sizeof(pub_unit_on_board),
                MQTT_PUBL_UNIT_ON_BOARD, device_id);
        if (MQTTSubscribe(&mclient, sub_command, MQTT_QOS_LVL,
                mqtt_rcvd_msg) < 0) {
		dbg_printf("%s:%d: MQTT Subscription failed\n",
			__func__, __LINE__);
		return false;
	}
	PRINTF("Subscribed to topic %s successful\n", sub_command);
	return true;
}

static bool mqtt_client_and_topic_init(void)
{
        MQTTClientInit(&mclient, &net, MQTT_TIMEOUT_MS,
                send_intr_buf, MQTT_SEND_SZ, recv_intr_buf, MQTT_RCV_SZ);
        if (!utils_get_device_id(device_id, MQTT_DEVICE_ID_SZ, NET_INTERFACE)) {
		dbg_printf("%s:%d: Can not retrieve device id\n",
			__func__, __LINE__);
		return false;
	}
	mqtt_conn_data.willFlag = MQTT_WILL;
	mqtt_conn_data.MQTTVersion = MQTT_PROTO_VERSION;
	mqtt_conn_data.clientID.cstring = device_id;
	mqtt_conn_data.username.cstring = NULL;
	mqtt_conn_data.password.cstring = NULL;
	mqtt_conn_data.keepAliveInterval = MQTT_KEEPALIVE_INT_SEC;
	mqtt_conn_data.cleansession = MQTT_CLEAN_SESSION;

        int res = MQTTConnect(&mclient, &mqtt_conn_data);
        if (res < 0) {
		dbg_printf("%s:%d: MQTT connect failed:%d\n",
			__func__, __LINE__, res);
		return false;
	}
	PRINTF("MQTT connect succeeded\n");
	if (!reg_pub_sub())
		return false;
	return true;
}

static bool __mqtt_net_connect(bool flag, bool cred_flag)
{
        if (!session.conn_valid && flag && cred_flag) {
		int ret = mqtt_net_connect();
		if (ret == -1) {
        		dbg_printf("%s:%d: Error in SSL handshake\n",
                                __func__, __LINE__);
                        return false;
                }
        	if (ret == -2) {
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

proto_result mqtt_set_own_auth(const uint8_t *cli_cert, uint32_t cert_len,
			const uint8_t *cli_key, uint32_t key_len)
{
	if ((cert_len == 0) || (key_len == 0))
		RETURN_ERROR("credentials is null", PROTO_INV_PARAM);
	if ((cli_cert == NULL) || (cli_key == NULL))
		RETURN_ERROR("certs are null", PROTO_INV_PARAM);

	if (!init_own_certs(cli_cert, cert_len, cli_key, key_len))
                RETURN_ERROR("Own certs initialization failed", PROTO_INV_PARAM);
	session.own_auth_valid = true;

	if (!__mqtt_net_connect(session.host_valid, session.remote_auth_valid))
	                RETURN_ERROR("remote connect failed", PROTO_ERROR);
	return PROTO_OK;
}

proto_result mqtt_set_remote_auth(const uint8_t *serv_creds, uint32_t cert_len)
{
	if ((serv_creds == NULL) || (cert_len == 0))
		RETURN_ERROR("root ca not valid", PROTO_INV_PARAM);

	if (!init_remote_certs(serv_creds, cert_len))
                RETURN_ERROR("ca certs initialization failed", PROTO_INV_PARAM);
	session.remote_auth_valid = true;

	if (!__mqtt_net_connect(session.host_valid, session.own_auth_valid))
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
        if (!__mqtt_net_connect(session.remote_auth_valid,
		session.own_auth_valid))
		RETURN_ERROR("remote connect failed", PROTO_ERROR);
	PRINTF("Remote ost is: %s\n", session.host);
	PRINTF("Remote port is: %s\n", session.port);
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

static proto_result initialize_send(const void *buf, uint32_t sz,
				proto_callback cb)
{
	if (!buf || sz == 0)
		RETURN_ERROR("buffer or size invalid", PROTO_INV_PARAM);

	if (!session.host_valid)
		RETURN_ERROR("mqtt_set_destination needs to be called first",
			PROTO_ERROR);
	if (!session.remote_auth_valid || !session.own_auth_valid)
		RETURN_ERROR("mqtt authorization APIs need to be called first",
			PROTO_ERROR);
	if (!session.conn_valid)
		RETURN_ERROR("No active connection", PROTO_ERROR);

	session.send_cb = cb;
	session.send_svc_id = CC_SERVICE_BASIC;
	msg.qos = MQTT_QOS_LVL;
	msg.retained = 0,
	msg.payload = (void *)buf;
	msg.payloadlen = sz;
	return PROTO_OK;
}

static proto_result mqtt_publish_msg(char *topic, const void *buf, uint32_t sz)
{
	if (MQTTPublish(&mclient, topic, &msg) == FAILURE) {
		dbg_printf("%s:%d: Publication failed on topic: %s\n",
			__func__, __LINE__, topic);
		INVOKE_SEND_CALLBACK(buf, sz, PROTO_SEND_FAILED);
		RETURN_ERROR("Send failed", PROTO_ERROR);
	}
	PRINTF("Published %u bytes on topic: %s\n", sz, topic);
	return PROTO_OK;
}

proto_result mqtt_send_msg_to_cloud(const void *buf, uint32_t sz,
				   proto_service_id svc_id, proto_callback cb,
				   char *topic)
{

	if (!topic)
		return PROTO_ERROR;
	(void)svc_id;
	proto_result res = initialize_send(buf, sz, cb);
	if (res != PROTO_OK)
		return res;
	snprintf(pub_command_rsp, sizeof(pub_command_rsp),
                MQTT_PUBL_CMD_RESPONSE, topic);
	res = mqtt_publish_msg(pub_command_rsp, buf, sz);
	if (res != PROTO_OK)
		return res;
	return PROTO_OK;
}

proto_result mqtt_send_status_msg_to_cloud(const void *buf, uint32_t sz,
					proto_callback cb)
{
	proto_result res = initialize_send(buf, sz, cb);
	if (res != PROTO_OK)
		return res;
	res = mqtt_publish_msg(pub_unit_on_board, buf, sz);
	if (res != PROTO_OK)
		return res;
	return PROTO_OK;

}

void mqtt_maintenance(void)
{
	if (MQTTYield(&mclient, MQTT_TIMEOUT_MS) == FAILURE)
		dbg_printf("%s:%d: MQTT operation failed\n",
			__func__, __LINE__);
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

void mqtt_initiate_quit(void)
{
	MQTTDisconnect(&mclient);
	mqtt_net_disconnect();
	mqtt_reset_state();
}

const uint8_t *mqtt_get_rcv_buffer_ptr(const void *msg)
{
	if (!msg)
		return NULL;
	return (uint8_t *)msg + PROTO_OVERHEAD_SZ;
}

uint32_t mqtt_get_polling_interval(void)
{
        return current_polling_interval;
}
