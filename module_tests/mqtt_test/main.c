/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include "dbg.h"
#include "sys.h"
#include "MQTTClient.h"

#define SEND_BUF_SZ			255
#define READ_BUF_SZ			255

static char host[] = "simpm-ea-iwk.thingspace.verizon.com";
static int port = 8883;
static char clientid[] = "stm32f4-ublox";
static char subtopic[] = "vztest/1";
static char pubtopic[] = "vztest/2";
static enum QoS qos = QOS1;

static char payload[SEND_BUF_SZ];
static MQTTMessage msg = { .qos = QOS1, .retained = 0, .payload = payload};
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

int main(int argc, char *argv[])
{
	sys_init();

	dbg_module_init();
	dbg_printf("Begin MQTT test:\n");

	unsigned char sendbuf[SEND_BUF_SZ];
	unsigned char readbuf[READ_BUF_SZ];

	Network n;
	MQTTClient c;

	ASSERT(paho_net_init(&n) == true);
	int r = paho_net_connect(&n, host, port);
	if (r == -1)
		fatal_err("Error in SSL handshake\n");
	if (r == -2)
		fatal_err("SSL handshake timeout\n");

	MQTTClientInit(&c, &n, 1000, sendbuf, SEND_BUF_SZ, readbuf, READ_BUF_SZ);

	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	data.willFlag = 0;
	data.MQTTVersion = 3;
	data.clientID.cstring = clientid;
	data.username.cstring = NULL;
	data.password.cstring = NULL;
	data.keepAliveInterval = 10;
	data.cleansession = 1;

	dbg_printf("Connecting to %s %d\n", host, port);
	int rc = MQTTConnect(&c, &data);
	dbg_printf("Connected %d\n", rc);

	dbg_printf("Subscribing to %s\n", subtopic);
	rc = MQTTSubscribe(&c, subtopic, qos, msg_arrived);
	dbg_printf("Subscribed %d\n", rc);

	while (1) {
		/* XXX: Maybe attempt reconnect on failure? */
		if (MQTTYield(&c, 1000) == FAILURE)
			dbg_printf("MQTT Fail\n");
		sys_delay(1000);

		if (recvd) {
			dbg_printf("[echo back]\n");
			if (MQTTPublish(&c, pubtopic, &msg) == FAILURE)
				dbg_printf("Pub fail\n");
			recvd = false;
		}
	}

	return 0;
}
