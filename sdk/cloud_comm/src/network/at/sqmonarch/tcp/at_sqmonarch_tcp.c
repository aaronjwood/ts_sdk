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
#define RING_BUF_SZ			2048
static uint8_t rx_buf[RING_BUF_SZ];

static volatile struct __attribute__((packed)) {
	uint8_t connected : 1;		/* True when connected to the server */
	uint8_t flag_peer_close : 1;	/* True: Closed by peer, False: Closed by us */
	uint8_t data_mode : 1;		/* True: TCP data being received over UART */
	buf_sz to_read;			/* Store the amount of URC data to read */
	rbuf *buf;			/* Handle to ring buffer to store received data */
} tcp_state = {
	.connected = false,
	.flag_peer_close = false,
	.data_mode = false,
	.to_read = 0,
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

/* Parse the length from "+SQNSRING: 1,<length>,<data bytes>". */
static void process_len(void)
{
	uint8_t data;
	at_core_read(&data, 1);
	buf_sz num_bytes = 0;
	while (data != ',') {		/* Read until the second ',' */
		num_bytes *= 10;
		num_bytes += (data - '0');
		at_core_read(&data, 1);
	}
	tcp_state.to_read = num_bytes;
	DEBUG_V0("%s: rx %d\n", __func__, tcp_state.to_read);
}

static void attempt_buffering_data(void)
{
	static bool process_data = false;
	if (!tcp_state.data_mode)
		return;

	/* Parse the length field */
	if (!process_data) {
		int p_idx = at_core_find_pattern(UART_BUF_BEGIN,
				(const uint8_t *)",", 1);
		if (p_idx < 0 || at_core_rx_available() < 2)
			return;
		process_len();
		process_data = true;
	}

	/* Parse the data field if enough data is available (+ CRLF) */
	if (at_core_rx_available() >= tcp_state.to_read + 2) {
		uint8_t data;
		for (buf_sz i = 0; i < tcp_state.to_read; i++) {
			at_core_read(&data, 1);
			rbuf_wb(tcp_state.buf, data);
		}
		tcp_state.to_read = 0;
		tcp_state.data_mode = false;
		process_data = false;

		/* Eat trailing CRLF */
		at_core_read(&data, 1);
		at_core_read(&data, 1);
		return;
	}
	return;
}

/*
 * +SQNSRING header, unlike other URCs, is not delimited by CRLF. For now, it is
 * easiest to retrieve it manually.
 */
static void attempt_capture_sqnsring(void)
{
	if (tcp_state.data_mode)
		return;

	const size_t str_len = strlen(at_urcs[TCP_RECV]);
	if (at_core_find_pattern(UART_BUF_BEGIN,
				(const uint8_t *)at_urcs[TCP_RECV],
				str_len) < 0)
		return;

	char dump[str_len];
	if (at_core_read((uint8_t *)dump, str_len) != str_len) {
		DEBUG_V0("%s: read err\n", __func__);
		return;
	}
	dump[str_len] = 0x00;
	tcp_state.data_mode = true;
	/* The read buffer now points to the beginning of the length field */
	attempt_buffering_data();
	return;
}

static void at_uart_callback(void)
{
	attempt_capture_sqnsring();
	attempt_buffering_data();
	if (!at_core_is_proc_rsp() && !at_core_is_proc_urc())
		at_core_process_urc(false);
}

static void process_urc(const char *urc, enum at_urc u_code)
{
	if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
		return;

	switch (u_code) {
	case TCP_CLOSED:
		tcp_state.connected = false;
		tcp_state.flag_peer_close = true;
		tcp_state.data_mode = false;
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
	tcp_state.data_mode = false;
	tcp_state.to_read = 0;

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
	at_core_wcmd(&tcp_commands[SOCK_CLOSE], true);
	tcp_state.connected = false;
	tcp_state.flag_peer_close = false;
	tcp_state.data_mode = false;
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
