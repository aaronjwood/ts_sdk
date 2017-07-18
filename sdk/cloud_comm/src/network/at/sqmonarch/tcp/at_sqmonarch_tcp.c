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

/*
 * Delay between successive commands in milisecond, datasheet recommends atleast
 * 20mS
 */
#define AT_COMM_DELAY_MS		20

static void at_uart_callback(void)
{
	if (!at_core_is_proc_urc() && !at_core_is_proc_urc())
		at_core_process_urc(false);
}

static void urc_callback(const char *urc)
{
	/* TODO: Process all communications URC */
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

	/*
	 * TODO: Configure and activate the PDP context
	 * TODO: Configure the TCP context
	 */

	return true;
}

int at_tcp_connect(const char *host, const char *port)
{
	/* TODO: Dial into the server using AT+SQNSD */
	return 0;
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
	/* TODO: Close connection */
}
