/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include "dbg.h"
#include "uart.h"
#include "at.h"
#include "platform.h"

const char *host = "httpbin.org";
const char *port = "80";

#define RECV_BUFFER	64
#define NUM_RETRIES	5
#define DELAY_MS	2000

static int tcp_connect()
{
	int count = 0;
	int s_id = -1;

	while (s_id < 0 && count < NUM_RETRIES) {
		s_id = at_tcp_connect(host, port);
		platform_delay(DELAY_MS);
		count++;
	}
	if (s_id < 0) {
		dbg_printf("Invalid socket id: %d, looping forever\n", s_id);
		ASSERT(0);
	}
	dbg_printf("Socket id is :%d\n", s_id);
	return s_id;
}

static void tcp_send(int s_id, uint8_t *send_buf, size_t len)
{
	int count = 0;
	int sent_data = 0;
	int res = 0;
	while (1) {
		res = at_tcp_send(s_id, (send_buf + sent_data), len - sent_data);
		if (res < 0) {
			if (count > NUM_RETRIES)
				break;
			else {
				count++;
				res = 0;
			}
			platform_delay(DELAY_MS);
		} else {
			sent_data += res;
			if (sent_data == len)
				break;
		}

	}
	if (sent_data <= 0) {
		dbg_printf("TCP send failed, looping forever\n");
		ASSERT(0);
	}
	dbg_printf("Sent data: %d\n", sent_data);

}

static void tcp_rcv(int s_id)
{
	int r_b;
	uint8_t count = 0;
	while (count < NUM_RETRIES) {
		r_b = at_read_available(s_id);
		printf("Read available:%d\n", r_b);
		if (r_b > 0)
			break;
		platform_delay(DELAY_MS);
		count++;
	}
	if (r_b <= 0) {
		dbg_printf("Read failed, code:%d, looping forever\n", r_b);
		ASSERT(0);
	}

	uint8_t read_buf[RECV_BUFFER];
	int bytes;
	while (1) {
		memset(read_buf, 0, RECV_BUFFER);
		bytes = at_tcp_recv(s_id, read_buf, RECV_BUFFER);
		if (bytes < 0) {
			dbg_printf("###### Read completed ######\n");
			break;
		}
		read_buf[bytes] = 0x0;
		printf("%s", (char *)read_buf);
	}
}

int main(int argc, char *argv[])
{
	platform_init();

	dbg_module_init();
	dbg_printf("Begin:\n");
	/* Step 1: modem init */
	dbg_printf("Initializing modem...\n");
	if (!at_init()) {
		dbg_printf("Modem init failed, looping forever\n");
		ASSERT(0);
	}
	dbg_printf("Initializing modem done...\n");
	/* step 2: tcp connect */
	int s_id = tcp_connect();
	dbg_printf("TCP connect done...\n");

	/* step 3: send data */
	char *send_buf = "GET \\ HTTP\\1.1\r\n";
	dbg_printf("Sending data:\n");
	size_t len = strlen(send_buf);
	tcp_send(s_id, (uint8_t *)send_buf, len);

	/* step 4: wait for the data */
	tcp_rcv(s_id);

	/* step 5: tcp close */
	at_tcp_close(s_id);

	while (1)
		;
	return 0;
}
