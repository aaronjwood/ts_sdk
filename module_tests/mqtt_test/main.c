/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include "dbg.h"
#include "sys.h"
#include "MQTTClient.h"

#define SEND_BUF_SZ			100
#define READ_BUF_SZ			100

char host[] = "test.mosquitto.org";
int port = 1883;			/* Use the unencrypted MQTT port */
char clientid[] = "stm32f4-ublox";
char topic[] = "vztest/1";
enum QoS qos = QOS2;

static void msg_arrived(MessageData *md)
{
	MQTTMessage *m = md->message;
	dbg_printf("%d\n", (int)m->payloadlen);
	for (uint8_t i = 0; i < m->payloadlen; i++)
		dbg_printf("%c", ((char *)m->payload)[i]);
	dbg_printf("\n");
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

	paho_net_init(&n);
	ASSERT(paho_net_connect(&n, host, port));
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

	dbg_printf("Subscribing to %s\n", topic);
	rc = MQTTSubscribe(&c, topic, qos, msg_arrived);
	dbg_printf("Subscribed %d\n", rc);

	while (1) {
		/* XXX: Maybe attempt reconnect on failure? */
		if (MQTTYield(&c, 1000) == FAILURE)
			dbg_printf("MQTT Fail\n");
	}

	return 0;
}
