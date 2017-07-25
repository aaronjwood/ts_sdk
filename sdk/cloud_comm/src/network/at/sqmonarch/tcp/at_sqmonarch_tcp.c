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

#define DATA_TMO_MS			110	/* Timeout to receive MAX_DATA_LEN bytes */

/* Store the received data into this buffer. at_tcp_read() reads from here. */
static uint8_t rx_buf[MAX_DATA_LEN];

static volatile struct __attribute__((packed)) {
	uint8_t connected : 1;		/* True when connected to the server */
	uint8_t flag_peer_close : 1;	/* True: Closed by peer, False: Closed by us */
	buf_sz unread;			/* Store the amount of unread data */
	rbuf *buf;			/* Handle to ring buffer to store received data */
} tcp_state = {
	.connected = false,
	.flag_peer_close = false,
	.unread = 0,
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

struct recv_opts {
	size_t len;
	uint8_t *buf;
};

#define SZ_OK			(sizeof("\r\nOK\r\n") - 1)
static void parse_data(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	/* Header information should have already been discarded */

	struct recv_opts *opts = (struct recv_opts *)data;

	/* Wait until all of the data has been received in the UART */
	uint32_t start = sys_get_tick_ms();
	while (at_core_rx_available() < opts->len + SZ_OK) {
		uint32_t now = sys_get_tick_ms();
		if (now - start > DATA_TMO_MS) {
			opts->len = 0;
			DEBUG_V0("%s: uart tcp data timeout\n", __func__);
			return;
		}
	}

	/* Read only the data into the user supplied buffer */
	if (at_core_read(opts->buf, opts->len) != opts->len) {
		opts->len = 0;
		return;
	}

	/* Discard the trailing "\r\nOK\r\n" */
	uint8_t ok_data[6];
	at_core_read(ok_data, SZ_OK);
}

static void at_uart_callback(void)
{
	if (!at_core_is_proc_urc() && !at_core_is_proc_urc())
		at_core_process_urc(false);
}

static void process_sqnsring_urc(const char *urc)
{
	const char *rbytes = urc + strlen(at_urcs[TCP_RECV]);
	buf_sz i = 0;
	buf_sz num_bytes = 0;
	while (rbytes[i] != '\r') {
		num_bytes *= 10;
		num_bytes += (rbytes[i] - '0');
		i++;
	}
	tcp_state.unread += num_bytes;
	if (tcp_state.unread > MAX_DATA_LEN) {
		DEBUG_V0("%s: modem buffer overflow\n", __func__);
		return;
	}
}

static bool process_urc(const char *urc, enum at_urc u_code)
{
	if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
		return false;

	switch (u_code) {
	case TCP_CLOSED:
		tcp_state.connected = false;
		tcp_state.flag_peer_close = true;
		DEBUG_V0("%s: TCP conn closed by peer\n", __func__);
		break;
	case TCP_RECV:
		process_sqnsring_urc(urc);
		break;
	default:
		return false;
	}
	return true;
}

static void urc_callback(const char *urc)
{
	if (process_urc(urc, TCP_CLOSED))
		return;

	process_urc(urc, TCP_RECV);
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

	char ip_addr[16];
	bool ip_res = at_tcp_get_ip(ip_addr);
	DEBUG_V0("IP:%s\n", ip_res ? ip_addr : "failed");

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
	snprintf(cmd, MAX_TCP_CMD_LEN, desc->comm_sketch, port, host);
	desc->comm = cmd;
	if (at_core_wcmd(desc, true) != AT_SUCCESS)
		return AT_CONNECT_FAILED;

	tcp_state.connected = true;
	tcp_state.flag_peer_close = false;
	tcp_state.unread = 0;

	DEBUG_V0("%s: socket("MODEM_SOCK_ID") created\n", __func__);
	const char sock_id[] = MODEM_SOCK_ID;
	return sock_id[0] - '0';
}

/* Send the hexadecimal representation of the data */
static bool send_chunk_as_hex(const uint8_t buf[], size_t len)
{
	if (at_core_wcmd(&tcp_commands[SOCK_SEND], false) != AT_SUCCESS)
		return false;

	static const uint8_t to_hex_lut[] = "0123456789ABCDEF";
	uint8_t hex_data[2];
	for (size_t i = 0; i < len; i++) {
		hex_data[0] = to_hex_lut[(buf[i] & 0xF0) >> 4];
		hex_data[1] = to_hex_lut[buf[i] & 0x0F];
		if (!at_core_write((uint8_t *)hex_data, sizeof(hex_data)))
			return false;
	}

	if (at_core_wcmd(&tcp_commands[SOCK_SEND_DATA], true) != AT_SUCCESS)
		return false;

	return true;
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
	if (!send_chunk_as_hex(buf, len))
		return AT_TCP_SEND_FAIL;

	return len;
}

int at_read_available(int s_id)
{
	const char sock_id[] = MODEM_SOCK_ID;
	if (s_id != sock_id[0] - '0')
		return AT_TCP_RCV_FAIL;

	return tcp_state.unread;
}

int at_tcp_recv(int s_id, uint8_t *buf, size_t len)
{
	const char sock_id[] = MODEM_SOCK_ID;
	if (s_id != sock_id[0] - '0' || buf == NULL)
		return AT_TCP_INVALID_PARA;

	if (len == 0)
		return 0;

	if (tcp_state.unread == 0) {
		errno = EAGAIN;
		return AT_TCP_RCV_FAIL;
	}

	char cmd[MAX_TCP_CMD_LEN];
	at_command_desc *desc = &tcp_commands[SOCK_RECV];

	len = (len > tcp_state.unread) ? tcp_state.unread : len;
	snprintf(cmd, MAX_TCP_CMD_LEN, desc->comm_sketch, len);
	desc->comm = cmd;

	desc->rsp_desc[0].data = &(struct recv_opts){len, buf};
	if (at_core_wcmd(desc, true) != AT_SUCCESS) {
		return AT_TCP_RCV_FAIL;
	}

	if (((struct recv_opts *)desc->rsp_desc[0].data)->len != len)
		return AT_TCP_RCV_FAIL;

	tcp_state.unread -= len;
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
	at_core_wcmd(&tcp_commands[SOCK_CLOSE], true);
	tcp_state.connected = false;
	tcp_state.flag_peer_close = false;
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
