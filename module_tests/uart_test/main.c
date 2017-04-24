/* Copyright(C) 2016 Verizon. All rights reserved. */

#include "dbg.h"
#include "sys.h"
#include "uart_hal.h"
#include "uart_util.h"
#include "ts_sdk_config.h"

#define SEND_TIMEOUT_MS		2000
#define IDLE_CHARS		5
#define MAX_RESP_SZ		600
static volatile bool received_response;
static uint8_t response[MAX_RESP_SZ];
static periph_t uart;

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
		int sz = uart_util_line_avail(header, trailer);
		sz = (sz > MAX_RESP_SZ) ? MAX_RESP_SZ : sz;
		if (sz == 0 || sz == -1)
			return;
		sz = uart_util_read(response, sz);
		ASSERT(sz != UART_BUF_INV_PARAM);
		response[sz] = 0x00;
		dbg_printf("Res:\n%s\n", response);
	} else if (event == UART_EVENT_RX_OVERFLOW) {
		dbg_printf("Buffer overflow!\n");
		uart_util_flush();
	}
}

int main(int argc, char *argv[])
{
	sys_init();

	dbg_module_init();
	const struct uart_pins pins = {
		.tx = MODEM_UART_TX_PIN,
		.rx = MODEM_UART_RX_PIN,
		.rts = MODEM_UART_RTS_PIN,
		.cts = MODEM_UART_CTS_PIN
	};

	const uart_config config = {
		.baud = MODEM_UART_BAUD_RATE,
		.data_width = MODEM_UART_DATA_WIDTH,
		.parity = MODEM_UART_PARITY,
		.stop_bits = MODEM_UART_STOP_BITS,
		.priority = MODEM_UART_IRQ_PRIORITY
	};
	uart = uart_init(&pins, &config);
	ASSERT(uart != NO_PERIPH);
	ASSERT(uart_util_init(uart, IDLE_CHARS) == true);
	uart_util_reg_callback(rx_cb);

	/*
	 * Note: It is assumed the modem has booted and is ready to receive
	 * commands at this point. If it isn't and HW flow control is enabled,
	 * the assertion over uart_tx is likely to fail implying the modem is
	 * not ready yet.
	 */
	dbg_printf("Begin:\n");

	/* ATE0 turns off echo in DCE for DTE commands */
	uint8_t echo_off[] = "ATE0\r";
	ASSERT(uart_tx(uart, echo_off, sizeof(echo_off), SEND_TIMEOUT_MS)
			== true);
	sys_delay(2000);		/* Wait for modem to process */

	/* Get rid of all responses that aren't surrounded by the header and
	 * trailer.
	 */
	uart_util_flush();

	/* Main test begins. */
	/* AT+CPIN? Checks if SIM card is ready. */
	uint8_t msg[] = "AT+CPIN?\r";
	ASSERT(uart_tx(uart, msg, sizeof(msg), SEND_TIMEOUT_MS) == true);

	while (1) {
		if (received_response) {
			/* Received a response. Wait and resend AT&V. */
			received_response = false;
			sys_delay(2000);
			ASSERT(uart_tx(uart, msg, sizeof(msg), SEND_TIMEOUT_MS)
					== true);
		}
	}
	return 0;
}
