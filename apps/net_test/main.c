/* Copyright(C) 2016 Verizon. All rights reserved. */

#include <string.h>
#include "dbg.h"
#include "uart.h"
#include "net.h"

extern void platform_init();
extern void platform_delay(uint32_t delay_ms);

const char *host = "httpbin.org";
const char *port = "80";

#define RECV_BUFFER	1024
#define NUM_RETRIES	5
#define DELAY_MS	2000

static void net_connect(mbedtls_net_context *ctx)
{
	int count = 0;
	int res = -1;

	while (res < 0 && count < NUM_RETRIES) {
		res = mbedtls_net_connect(ctx, host, port,
			MBEDTLS_NET_PROTO_TCP);
		platform_delay(DELAY_MS);
		count++;
	}
	if (res != 0) {
		dbg_printf("Connect failed with error: %d, looping forever\n",
			res);
		ASSERT(0);
	}
	dbg_printf("Socket id is :%d\n", ctx->fd);
}

static void net_send(mbedtls_net_context *ctx, uint8_t *send_buf, size_t len)
{
	int count = 0;
	int sent_data = 0;
	int res = 0;
	while (1) {
		res = mbedtls_net_send(ctx, (send_buf + sent_data),
				len - sent_data);
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
		mbedtls_net_free(ctx);
		dbg_printf("net send failed, looping forever\n");
		ASSERT(0);
	}
	dbg_printf("Sent data: %d\n", sent_data);

}

int main(int argc, char *argv[])
{
	platform_init();

	dbg_module_init();
	dbg_printf("Initializing mbedtls, it may take few seconds\n");

	/* step 1: Initializes network stack */
	mbedtls_net_context ctx;
	if (!mbedtls_net_init(&ctx)) {
		dbg_printf("Init failed, looping forever\n");
		ASSERT(0);
	}

	/* step 2: network connect */
	net_connect(&ctx);
	dbg_printf("net connect done...\n");

	/* step 3: send data */
	dbg_printf("Sending data:\n");
	char *send_buf = "GET \\ HTTP\\1.1\r\n";
	size_t len = strlen(send_buf);
	net_send(&ctx, (uint8_t *)send_buf, len);

	/* step 4: receive data */
	uint8_t read_buf[RECV_BUFFER];
	int bytes;
	uint8_t count = 0;
	while (count < NUM_RETRIES) {
		bytes = mbedtls_net_recv(&ctx, read_buf, RECV_BUFFER);
		if (bytes > 0)
			break;
		platform_delay(DELAY_MS);
		count++;
	}
	if (bytes <= 0) {
		dbg_printf("Read failed, code:%d, looping forever\n", bytes);
		ASSERT(0);
	}
	read_buf[bytes] = 0x0;
	printf("Read Bytes: %d\n", bytes);
	printf("%s\n", (char *)read_buf);

	mbedtls_net_free(&ctx);
	while (1) {
	}
	return 0;
}
