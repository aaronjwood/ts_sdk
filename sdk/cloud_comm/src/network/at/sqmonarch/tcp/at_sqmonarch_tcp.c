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

#define MAX_TCP_CMD_LEN			64

/*
 * Delay between successive commands in milisecond, datasheet recommends atleast
 * 20mS
 */
#define AT_COMM_DELAY_MS		20

#define IP_ADDR_LEN			16

/* Sequans Monarch supports only 1500 bytes of outgoing data at a time */
#define MAX_SEND_DATA_LEN		1500

static char modem_ip_addr[IP_ADDR_LEN];

static volatile struct __attribute__((packed)) {
	uint8_t connected : 1;		/* True when connected to the server */
	uint8_t receiving : 1;		/* True while receiving data on the UART */
	uint8_t flag_peer_close : 1;	/* True: Closed by peer, False: Closed by us */
	buf_sz unread;			/* Store the amount of unread data */
	buf_sz wanted_bytes;		/* Amount of data being sent by +SQNSRING */
} tcp_state;

static void parse_ip_addr(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	char *rcv_bytes = (char *)rcv_rsp + strlen(stored_rsp);
	uint8_t i = 0;
	while (rcv_bytes[i] != '"') {
		modem_ip_addr[i] = rcv_bytes[i];
		i++;
	}
}

#if 0
static void attempt_sqsring_retrieval(void)
{
	if (tcp_state.receiving) {
		if (at_core_rx_available() >= tcp_state.wanted_bytes) {
			tcp_state.wanted_bytes = 0;
			tcp_state.receiving = false;
		}
	}
}
#endif

static void at_uart_callback(void)
{
	if (!at_core_is_proc_urc() && !at_core_is_proc_urc())
		at_core_process_urc(false);
}

static bool process_urc(const char *urc, enum at_urc u_code)
{
	if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
		return false;

	switch (u_code) {
	case TCP_CLOSED:
		tcp_state.connected = false;
		tcp_state.receiving = false;
		tcp_state.flag_peer_close = true;
		DEBUG_V0("%s: TCP conn closed by peer\n", __func__);
		break;
	/* TODO: Process +SQNSRING URC - See SMS code for hints */
	default:
		return false;
	}
	return true;
}

static void urc_callback(const char *urc)
{
	if (process_urc(urc, TCP_CLOSED))
		return;

	/* TODO: Process +SQNSRING URC */
}

bool at_init()
{
	tcp_state.connected = false;
	tcp_state.receiving = false;
	tcp_state.flag_peer_close = false;
	tcp_state.unread = 0;

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
	tcp_state.receiving = false;
	tcp_state.flag_peer_close = false;
	tcp_state.unread = 0;

	DEBUG_V0("%s: socket("MODEM_SOCK_ID") created\n", __func__);
	const char sock_id[] = MODEM_SOCK_ID;
	return sock_id[0] - '0';
}

static bool send_chunk(uint8_t *buf, size_t len)
{
	if (len > MAX_SEND_DATA_LEN)
		return false;

	if (at_core_wcmd(&tcp_commands[SOCK_SEND], false) != AT_SUCCESS)
		return false;

	if (!at_core_write(buf, len))
		return false;

	if (at_core_wcmd(&tcp_commands[SOCK_SEND_DATA], true) != AT_SUCCESS)
		return false;

	return true;
}

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

	/* Send in chunks of MAX_SEND_DATA_LEN */
	size_t remaining = len;
	uint8_t *data = (uint8_t *)buf;
	while (remaining >= MAX_SEND_DATA_LEN) {
		if (!tcp_state.connected)
			return AT_TCP_CONNECT_DROPPED;

		if (!send_chunk(data, MAX_SEND_DATA_LEN))
			return AT_TCP_SEND_FAIL;

		remaining -= MAX_SEND_DATA_LEN;
		data += MAX_SEND_DATA_LEN;
	}

	if (!tcp_state.connected)
		return AT_TCP_CONNECT_DROPPED;

	if (!send_chunk(data, remaining))
		return AT_TCP_SEND_FAIL;

	if (!tcp_state.connected)
		return AT_TCP_CONNECT_DROPPED;
	return len;
}

int at_read_available(int s_id)
{
	const char sock_id[] = MODEM_SOCK_ID;
	if (s_id != sock_id[0] - '0')
		return AT_TCP_RCV_FAIL;
	if (!tcp_state.connected) {
		if (!tcp_state.flag_peer_close)
			return 0;
		if (tcp_state.unread > 0)
			return tcp_state.unread;
		return AT_TCP_RCV_FAIL;
	}
	return tcp_state.unread;
}

static int read_socket(uint8_t *buf, size_t len)
{
	if (tcp_state.unread > 0) {
		int avail = at_core_rx_available();
		if (avail >= len) {
			int rdb = at_core_read(buf, len);
			if (rdb < 0)
				return AT_TCP_RCV_FAIL;
			tcp_state.unread -= rdb;
			return rdb;
		}
	}
	errno = EAGAIN;
	return AT_TCP_RCV_FAIL;
}

static int read_inter_buf(uint8_t *buf, size_t len)
{
	if (tcp_state.unread > 0) {
		int rdb = at_core_read(buf, len);
		if (rdb < 0)
			return AT_TCP_RCV_FAIL;
		tcp_state.unread -= rdb;
		return rdb;
	}
	DEBUG_V0("%s: tcp not connected to recv\n", __func__);
	return AT_TCP_RCV_FAIL;
}

int at_tcp_recv(int s_id, uint8_t *buf, size_t len)
{
	const char sock_id[] = MODEM_SOCK_ID;
	if (s_id != sock_id[0] - '0' || buf == NULL)
		return AT_TCP_INVALID_PARA;

	if (len == 0)
		return 0;

	if (!tcp_state.connected) {
		if (!tcp_state.flag_peer_close)
			return 0;
		return read_inter_buf(buf, len);
	}

	return read_socket(buf, len);
}

void at_tcp_close(int s_id)
{
	const char sock_id[] = MODEM_SOCK_ID;
	if (s_id != sock_id[0] - '0' || !tcp_state.connected)
		return;
	at_core_wcmd(&tcp_commands[SOCK_CLOSE], true);
	tcp_state.connected = false;
	tcp_state.receiving = false;
	tcp_state.flag_peer_close = false;
}

bool at_tcp_get_ip(char *ip)
{
	if (ip == NULL)
		return false;
	if (at_core_wcmd(&tcp_commands[GET_IP_ADDR], true) != AT_SUCCESS)
		return false;
	if (modem_ip_addr[0] == 0x00)
		return false;
	memset(ip, 0, IP_ADDR_LEN);
	strncpy(ip, modem_ip_addr, IP_ADDR_LEN);

	return true;
}
