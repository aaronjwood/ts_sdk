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

static char modem_ip_addr[IP_ADDR_LEN];

static bool connected;

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
		connected = false;
		DEBUG_V0("%s: TCP conn closed\n", __func__);
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
	connected = false;

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
	if (connected)
		return AT_CONNECT_FAILED;
	char cmd[MAX_TCP_CMD_LEN];
	at_command_desc *desc = &tcp_commands[SOCK_DIAL];
	snprintf(cmd, MAX_TCP_CMD_LEN, desc->comm_sketch, port, host);
	desc->comm = cmd;
	if (at_core_wcmd(desc, true) != AT_SUCCESS)
		return AT_CONNECT_FAILED;
	connected = true;
	DEBUG_V0("%s: socket("MODEM_SOCK_ID") created\n", __func__);
	const char sock_id[] = MODEM_SOCK_ID;
	return sock_id[0] - '0';
}

int at_tcp_send(int s_id, const uint8_t *buf, size_t len)
{
	/* TODO: Send data in chunks of 1500 bytes using AT+SQNSSEND=1 */
	return 0;
}

int at_read_available(int s_id)
{
	return 0;
}

int at_tcp_recv(int s_id, uint8_t *buf, size_t len)
{
	/* TODO: Read internal UART buffer */
	return 0;
}

void at_tcp_close(int s_id)
{
	if (s_id < 0 || !connected)
		return;
	at_core_wcmd(&tcp_commands[SOCK_CLOSE], true);
	connected = false;
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
