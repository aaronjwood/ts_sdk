/* Copyright(C) 2016 Verizon. All rights reserved. */

#include "dbg.h"
#include "uart.h"
#include "platform.h"

#define SEND_TIMEOUT_MS		2000
#define IDLE_CHARS		5
#define MAX_RESP_SZ		600
static volatile bool received_response;
static uint8_t response[MAX_RESP_SZ];

/* UART receive callback. */
static void rx_cb(callback_event event)
{
	if (event == UART_EVENT_RECVD_BYTES) {
		/*
		 * Note: If echo is turned on in the modem, the command just
		 * sent will be seen as the first response. It is up to the AT
		 * layer to keep track of this.
		 */
		received_response = true;
		char header[] = "\r\n";
		char trailer[] = "\r\n";
		int sz = uart_line_avail(header, trailer);
		sz = (sz > MAX_RESP_SZ) ? MAX_RESP_SZ : sz;
		if (sz == 0 || sz == -1)
			return;
		sz = uart_read(response, sz);
		ASSERT(sz != UART_READ_ERR);
		response[sz] = 0x00;
		dbg_printf("Res:\n%s\n", response);
	} else if (event == UART_EVENT_RX_OVERFLOW) {
		dbg_printf("Buffer overflow!\n");
		uart_flush_rx_buffer();
	}
}

int main(int argc, char *argv[])
{
	platform_init();

	dbg_module_init();
	ASSERT(uart_module_init(UART_EN_HW_CTRL, IDLE_CHARS) == true);
	uart_set_rx_callback(rx_cb);

	/*
	 * Note: It is assumed the modem has booted and is ready to receive
	 * commands at this point. If it isn't and HW flow control is enabled,
	 * the assertion over uart_tx is likely to fail implying the modem is
	 * not ready yet.
	 */
	dbg_printf("Begin:\n");

	uint8_t echo_off[] = "ATE0\r";	/* ATE0 turns off echo in DCE for DTE commands */
	ASSERT(uart_tx(echo_off, sizeof(echo_off), SEND_TIMEOUT_MS) == true);
	platform_delay(2000);		/* Wait for modem to process */

	/* Get rid of all responses that aren't surrounded by the header and
	 * trailer.
	 */
	uart_flush_rx_buffer();

	/* Main test begins. */
	uint8_t msg[] = "AT+CPIN?\r";	/* AT+CPIN? Checks if SIM card is ready. */
	ASSERT(uart_tx(msg, sizeof(msg), SEND_TIMEOUT_MS) == true);

	while (1) {
		if (received_response) {
			/* Received a response. Wait and resend AT&V. */
			received_response = false;
			platform_delay(2000);
			ASSERT(uart_tx(msg, sizeof(msg), SEND_TIMEOUT_MS) == true);
		}
	}
	return 0;
}
