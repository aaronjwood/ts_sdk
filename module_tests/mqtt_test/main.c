/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include "dbg.h"
#include "sys.h"
#include "MQTTClient.h"

/*
 * XXX: In future implementations, paho_port_generic.h should not be referenced
 * directly. Instead, the middleware header should be included.
 */
#include "paho_port_generic.h"

/* Client certificate and private key */
#include "client-crt-1801.h"
#include "client-key-1801.h"

#define SEND_BUF_SZ			255
#define READ_BUF_SZ			255

#define QOS_LVL				QOS1

static char host[] = "simpm-ea-iwk.thingspace.verizon.com";
static int port = 8883;
static char clientid[] = "stm32f4-ublox";
static char subtopic[] = "vztest/recv";
static char pubtopic[] = "vztest/send";

static char payload[SEND_BUF_SZ];
static MQTTMessage msg = {.qos = QOS_LVL, .retained = 0, .payload = payload};
static bool recvd;

static void msg_arrived(MessageData *md)
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

static void attempt_tls_tcp_conn(const char *host, uint16_t port)
{
	int r = mqtt_net_connect(host, port);
	if (r == -1)
		fatal_err("Error in SSL handshake\n");
	if (r == -2)
		fatal_err("SSL handshake timeout\n");
}

static void attempt_mqtt_conn(MQTTClient *cl, MQTTPacket_connectData *data)
{
	if (MQTTConnect(cl, data) < 0)
		fatal_err("MQTT connect failed\n");
	dbg_printf("MQTT connect succeeded\n");
}

int main(void)
{
	sys_init();

	dbg_module_init();
	dbg_printf("Begin MQTT test:\n");

	unsigned char sendbuf[SEND_BUF_SZ];
	unsigned char readbuf[READ_BUF_SZ];

	Network net;
	MQTTClient cl;
	auth_creds creds = {
		.cl_cert_len = sizeof(client_cert),
		.cl_cert = client_cert,
		.cl_key_len = sizeof(client_key),
		.cl_key = client_key
	};

	if (!mqtt_net_init(&net, &creds))
		fatal_err("Could not initialize the TLS context\n");

	attempt_tls_tcp_conn(host, port);

	dbg_printf("Connected to server\n");

	MQTTClientInit(&cl, &net, 1000, sendbuf, SEND_BUF_SZ,
			readbuf, READ_BUF_SZ);

	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	data.willFlag = 0;
	data.MQTTVersion = 3;
	data.clientID.cstring = clientid;
	data.username.cstring = NULL;
	data.password.cstring = NULL;
	data.keepAliveInterval = 10;
	data.cleansession = 1;

	dbg_printf("Connecting to %s %d\n", host, port);
	attempt_mqtt_conn(&cl, &data);

	dbg_printf("Subscribing to %s\n", subtopic);
	int rc = MQTTSubscribe(&cl, subtopic, QOS_LVL, msg_arrived);
	dbg_printf("Subscribed %d\n", rc);

	/*
	 * XXX: Current model is to connect to the server and stay connected
	 * using "keep alive". This allows the server to send messages to the
	 * device at any time.
	 * Experiment with a wakeup-connect-receive-send-disconnect-sleep model.
	 */
	while (1) {
		if (MQTTYield(&cl, 1000) == FAILURE) {
			dbg_printf("MQTT Fail\n");
			MQTTDisconnect(&cl);
			mqtt_net_disconnect();
			attempt_tls_tcp_conn(host, port);
			dbg_printf("Reconnecting to %s %d\n", host, port);
			attempt_mqtt_conn(&cl, &data);
		}
		sys_delay(1000);

		if (recvd) {
			dbg_printf("[echo back]\n");
			if (MQTTPublish(&cl, pubtopic, &msg) == FAILURE)
				dbg_printf("Pub fail\n");
			recvd = false;
		}
	}

	return 0;
}
