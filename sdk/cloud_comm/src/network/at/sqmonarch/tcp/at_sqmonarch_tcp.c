/* Copyright (C) 2017 Verizon. All rights reserved. */
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "sys.h"
#include "dbg.h"
#include "at_core.h"
#include "at_tcp.h"
#include "at_modem.h"
#include "ts_sdk_modem_config.h"
#include "at_sqmonarch_tcp_command.h"
#include "rbuf.h"

#define MAX_TCP_CMD_LEN			64

/*
 * Delay between successive commands in milisecond, datasheet recommends atleast
 * 20mS
 */
#define AT_COMM_DELAY_MS		20

#define IP_ADDR_LEN			16

/* Sequans Monarch supports only 1500 bytes of outgoing data at a time */
#define MAX_DATA_LEN			1500

/* Store the received data into this buffer. at_tcp_recv() reads from here. */
#define RING_BUF_SZ			2048

static uint8_t rx_buf[RING_BUF_SZ];

static const uint8_t disconn_str[] = "\r\nNO CARRIER\r\n";

static volatile struct __attribute__((packed)) {
	uint8_t connected : 1;		/* True when connected to the server */
	uint8_t flag_peer_close : 1;	/* True: Closed by peer, False: Closed by us */
	rbuf *buf;			/* Handle to ring buffer to store received data */
} tcp_state = {
	.connected = false,
	.flag_peer_close = false,
	.buf = NULL
};

static void parse_ip_addr(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	char *rcv_bytes = (char *)rcv_rsp + strlen(stored_rsp);
	if (rcv_bytes[0] != ',')
		return;		/* PDP context is probably not active; no IP */
	rcv_bytes += 2;		/* Skip the ',' and the '"' */
	uint8_t i = 0;
	while (rcv_bytes[i] != '"') {
		((char *)data)[i] = rcv_bytes[i];
		i++;
	}
}

static void at_uart_callback(void)
{
	/* URCs aren't received in modem's online mode operation */
	if (!tcp_state.connected) {
		if (!at_core_is_proc_rsp() && !at_core_is_proc_urc())
			at_core_process_urc(false);
		return;
	}

	/* Complete disconnect string found in the buffer */
	if (at_core_find_pattern(UART_BUF_BEGIN, disconn_str,
				sizeof (disconn_str) - 1) >= 0) {
		tcp_state.connected = false;
		tcp_state.flag_peer_close = true;
		DEBUG_V0("%s: TCP conn closed by peer\n", __func__);
	} else { /* Store data into the ring buffer */
		buf_sz avail_bytes = at_core_rx_available();
		if (avail_bytes > 0) {
			uint8_t data;
			for (buf_sz i = 0; i < avail_bytes; i++) {
				at_core_read(&data, 1);
				rbuf_wb(tcp_state.buf, data);
			}
		}
	}
}

static void process_urc(const char *urc, enum at_urc u_code)
{
	if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
		return;

	switch (u_code) {
	case TCP_CLOSED:
		tcp_state.connected = false;
		tcp_state.flag_peer_close = true;
		DEBUG_V0("%s: TCP conn closed by peer\n", __func__);
		break;
	default:
		break;
	}
}

static void urc_callback(const char *urc)
{
	process_urc(urc, TCP_CLOSED);
}

bool at_init()
{
	/* XXX: Call at_core_emu_hwflctrl() here, before at_core_init() */
	if (!at_core_emu_hwflctrl(MODEM_EMULATED_RTS, MODEM_EMULATED_CTS))
		return false;

	if (!at_core_init(at_uart_callback, urc_callback, AT_COMM_DELAY_MS))
		return false;

	/*
	 * The Sequans Monarch does a system restart as part of the power up
	 * sequence (essentially a hardware reset) and does not require an
	 * additional restart here. However, the modem does need to be configured.
	 */
	if (!at_modem_configure()) {
		dbg_printf("%s: failed to configure modem\n", __func__);
		return false;
	}

	/* Activate PDP context and configure TCP socket */
	at_ret_code res = at_core_wcmd(&tcp_commands[PDP_ACT], true);
	CHECK_SUCCESS(res, AT_SUCCESS, false);

	res = at_core_wcmd(&tcp_commands[SOCK_CONF], true);
	CHECK_SUCCESS(res, AT_SUCCESS, false);

	res = at_core_wcmd(&tcp_commands[SOCK_CONF_EXT], true);
	CHECK_SUCCESS(res, AT_SUCCESS, false);

#ifdef DEBUG_AT_LIB
	char ip_addr[16];
	bool ip_res = at_tcp_get_ip(ip_addr);
	DEBUG_V0("IP:%s\n", ip_res ? ip_addr : "failed");
#endif

	/* Initialize the ring buffer for TCP reads */
	tcp_state.buf = rbuf_init(sizeof(rx_buf), rx_buf);
	if (tcp_state.buf == NULL) {
		DEBUG_V0("%s: failed to initialize receive buffer\n", __func__);
		return false;
	}

	return true;
}

int at_tcp_connect(const char *host, const char *port)
{
	if (tcp_state.connected)
		return AT_CONNECT_FAILED;

	char cmd[MAX_TCP_CMD_LEN];
	at_command_desc *desc = &tcp_commands[SOCK_DIAL];
	snprintf(cmd, sizeof(cmd), desc->comm_sketch, port, host);
	desc->comm = cmd;
	if (at_core_wcmd(desc, true) != AT_SUCCESS)
		return AT_CONNECT_FAILED;

	tcp_state.connected = true;
	tcp_state.flag_peer_close = false;

	DEBUG_V0("%s: socket("MODEM_SOCK_ID") created\n", __func__);
	const char sock_id[] = MODEM_SOCK_ID;
	return sock_id[0] - '0';
}

/* Can send only MAX_DATA_LEN bytes at a time */
int at_tcp_send(int s_id, const uint8_t *buf, size_t len)
{
	const char sock_id[] = MODEM_SOCK_ID;
	if (s_id != sock_id[0] - '0' || len == 0 || buf == NULL)
		return AT_TCP_INVALID_PARA;

	if (!tcp_state.connected) {
		if (!tcp_state.flag_peer_close)
			return 0;
		DEBUG_V0("%s: tcp not connected to send\n", __func__);
		return AT_TCP_SEND_FAIL;
	}

	if (!tcp_state.connected)
		return AT_TCP_CONNECT_DROPPED;

	len = (len > MAX_DATA_LEN) ? MAX_DATA_LEN : len;
	for (size_t i = 0; i < len; i++)
		if (tcp_state.connected) {
			if (!at_core_write((uint8_t *)&buf[i], 1)) {
				DEBUG_V0("%s: write failed", __func__);
				return AT_TCP_SEND_FAIL;
			}
		} else {
			DEBUG_V0("%s: tcp connection lost in middle of write\n",
					__func__);
			return AT_TCP_CONNECT_DROPPED;
		}

	return len;
}

int at_read_available(int s_id)
{
	const char sock_id[] = MODEM_SOCK_ID;
	if (s_id != sock_id[0] - '0')
		return AT_TCP_RCV_FAIL;

	return rbuf_unread(tcp_state.buf);
}

int at_tcp_recv(int s_id, uint8_t *buf, size_t len)
{
	const char sock_id[] = MODEM_SOCK_ID;
	if (s_id != sock_id[0] - '0' || buf == NULL)
		return AT_TCP_INVALID_PARA;

	if (len == 0)
		return 0;

	size_t unread = rbuf_unread(tcp_state.buf);
	if (unread == 0) {
		errno = EAGAIN;
		return AT_TCP_RCV_FAIL;
	}

	len = (len > unread) ? unread : len;

	for (size_t i = 0; i < len; i++)
		if (!rbuf_rb(tcp_state.buf, &buf[i])) {
			DEBUG_V0("%s: read error\n", __func__);
			return AT_TCP_RCV_FAIL;
		}

	return len;
}

void at_tcp_close(int s_id)
{
	const char sock_id[] = MODEM_SOCK_ID;
	if (s_id != sock_id[0] - '0')
		return;
	if (!tcp_state.connected) {
		DEBUG_V0("%s: socket already closed\n", __func__);
		return;
	}

	DEBUG_V0("%s: closing tcp socket\n", __func__);
	/*
	 * Arbitrary wait time before the escape sequence, to enable its
	 * detection. AT manual is unclear about this.
	 */
	sys_delay(10);
	at_core_wcmd(&tcp_commands[SOCK_CLOSE], true);
	tcp_state.connected = false;
	tcp_state.flag_peer_close = false;
	rbuf_clear(tcp_state.buf);
}

bool at_tcp_get_ip(char *ip)
{
	if (ip == NULL)
		return false;
	memset(ip, 0, IP_ADDR_LEN);
	tcp_commands[GET_IP_ADDR].rsp_desc[0].data = ip;
	if (at_core_wcmd(&tcp_commands[GET_IP_ADDR], true) != AT_SUCCESS)
		return false;
	if (ip[0] == 0x00)
		return false;

	return true;
}
