/* Copyright (c) 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdio.h>
#include "sys.h"
#include "gpio_hal.h"
#include "at_core.h"
#include "at_modem.h"
#include "ts_sdk_modem_config.h"

#define AT_UART_TX_WAIT_MS		10000
#define IDLE_CHARS			10

/*
 * Waiting arbitrarily as we do not know for how many
 * bytes we will be waiting for when modem does not send wrong response in
 * totality
 */
#define RSP_BUF_DELAY			2000 /* In mili seconds */

static uint32_t at_comm_delay_ms;
static const char *rsp_header = "\r\n";
static const char *rsp_trailer = "\r\n";
static const char *err_str = "\r\nERROR\r\n";

typedef struct __attribute__((packed)) {
	unsigned int proc_rsp : 1;	/* '1' while processing a response */
	unsigned int proc_urc : 1;	/* '1' while processing URC outside of
					 * the interrupt / callback context.
					 */
	unsigned int waiting_resp : 1;	/* '1' while waiting for a response */
} at_core_state;

static volatile at_core_state state;
static volatile bool process_rsp;
static at_rx_callback serial_rx_callback;
static at_urc_callback urc_callback;

/* Handle to the UART peripheral connecting the modem and the MCU */
static periph_t uart;

static void __at_dump_buffer(const char *buf, buf_sz len)
{
        if (!buf) {
                buf_sz bytes = uart_util_available();
                if (bytes > 0) {
                        uint8_t temp_buf[bytes];
                        uart_util_read(temp_buf, bytes);
                        for (uint8_t i = 0; i < bytes; i++)
                                DEBUG_V0("0x%x, ", temp_buf[i]);
                }
        } else {
                for (uint8_t i = 0; i < len; i++)
                        DEBUG_V0("0x%x, ", buf[i]);
        }
        DEBUG_V0("\n");
}

static void __at_dump_wrng_rsp(char *rsp_buf, buf_sz read_bytes, const char *rsp)
{
#ifdef DEBUG_WRONG_RSP
        DEBUG_V0("%s: Printing received wrong response:\n", __func__);
        __at_dump_buffer(rsp_buf, read_bytes);
        DEBUG_V0("\n");
        DEBUG_V0("%s: Printing expected response:\n", __func__);
        __at_dump_buffer(rsp, strlen(rsp));
        DEBUG_V0("\n");
#endif
}

static at_ret_code __at_handle_error_rsp(char *rsp_buf, buf_sz read_bytes,
                                        const char *rsp, const char *err)
{
        at_ret_code result = AT_SUCCESS;
        if (err) {
                if (strncmp(rsp_buf, err, strlen(err)) != 0) {
                        if (strncmp(rsp_buf, err_str,
                                strlen(err_str)) == 0) {
                                DEBUG_V0("%s: General Failure: %s\n",
                                        __func__, rsp_buf);
                                result = AT_FAILURE;
                        } else {
                                DEBUG_V0("%s: wrong rsp: %s\n",
                                        __func__, rsp_buf);
                                result = AT_WRONG_RSP;
                        }
                } else
                        result = AT_FAILURE;
        } else {
                DEBUG_V0("%s: wrong response\n", __func__);
                __at_dump_wrng_rsp(rsp_buf, read_bytes, rsp);
                result = AT_WRONG_RSP;
        }
        return result;
}

static at_ret_code __at_wait_for_rsp(uint32_t *timeout)
{
	state.waiting_resp = true;
	at_ret_code result = AT_SUCCESS;
	uint32_t start = sys_get_tick_ms();
	uint32_t end = start;
	while (!process_rsp) {
		end = sys_get_tick_ms();
		if ((end - start) > *timeout) {
			result = AT_RSP_TIMEOUT;
			DEBUG_V1("%s: RSP_TIMEOUT: out of %lu, waited %lu\n",
					__func__, *timeout, (end - start));
			*timeout = 0;
			break;
		}
	}
	state.waiting_resp = false;
	process_rsp = false;
	if (*timeout != 0 && ((end - start) < *timeout))
		*timeout = *timeout - (end - start);
	return result;
}

static void at_core_cleanup(void)
{
	at_core_process_urc(true);
	at_core_clear_rx();
}

static at_ret_code __at_comm_send_and_wait_rsp(char *comm, uint16_t len,
                                                uint32_t *timeout)
{
        /* Process urcs if any before we send down new command
         * and empty buffer
         */
        at_core_cleanup();
        CHECK_NULL(comm, AT_FAILURE);

        if (!at_core_write((uint8_t *)comm, len)) {
                DEBUG_V0("%s: uart tx fail\n", __func__);
                return AT_TX_FAILURE;
        }
        return __at_wait_for_rsp(timeout);
}

static at_ret_code __at_wait_for_bytes(buf_sz *rcv_bytes,
                                        buf_sz target_bytes,
                                        uint32_t *timeout)
{
	while ((*rcv_bytes < target_bytes) && (*timeout > 0)) {
		/* Modem may be busy and could not send whole response that
		 * we are looking for, wait here to give another chance
		 */
		__at_wait_for_rsp(timeout);
		state.waiting_resp = true;
		/* New total bytes available to read */
		*rcv_bytes = uart_util_available();

	}
	state.waiting_resp = true;
	if (*timeout == 0) {
		*rcv_bytes = uart_util_available();
		if (*rcv_bytes < target_bytes)
			return AT_FAILURE;
	}
	DEBUG_V1("%s: data available (%u), wanted (%u), waited for %lu time\n",
			__func__, *rcv_bytes, target_bytes, *timeout);
	return AT_SUCCESS;
}

static at_ret_code __at_uart_waited_read(char *buf, buf_sz wanted,
                                        uint32_t *timeout)
{
        buf_sz rcvd = uart_util_available();
        if (__at_wait_for_bytes(&rcvd, wanted, timeout) != AT_SUCCESS) {
                DEBUG_V0("%s: bytes not present\n", __func__);
                return AT_FAILURE;
        }

        if (uart_util_read((uint8_t *)buf, wanted) < wanted) {
                DEBUG_V0("%s: Unlikely read error\n", __func__);
                return AT_FAILURE;
        }
        return AT_SUCCESS;
}

#if defined(MODEM_EMULATED_CTS) && defined(MODEM_EMULATED_RTS)
static pin_name_t m_rts = NC;
static pin_name_t m_cts = NC;
static bool emu_hwflctrl(pin_name_t rts, pin_name_t cts)
{
	if (rts == NC || cts == NC)
		return false;

	m_rts = rts;
	m_cts = cts;

	gpio_config_t flctrl_pins = {
		.dir = OUTPUT,
		.pull_mode = PP_NO_PULL,
		.speed = SPEED_HIGH
	};

	if (!gpio_init(m_rts, &flctrl_pins))
		return false;

	gpio_write(m_rts, PIN_HIGH);

	flctrl_pins.dir = INPUT;
	if (!gpio_init(m_cts, &flctrl_pins))
		return false;

	return true;
}
#endif

#define CTS_TIMEOUT_MS			1500
bool at_core_write(uint8_t *buf, uint16_t len)
{
#if defined(MODEM_EMULATED_CTS) && defined(MODEM_EMULATED_RTS)
	if (m_rts != NC && m_cts != NC) {
		gpio_write(m_rts, PIN_LOW);
		uint32_t start = sys_get_tick_ms();
		while (gpio_read(m_cts) == PIN_HIGH) {
			uint32_t now = sys_get_tick_ms();
			if (now - start > CTS_TIMEOUT_MS)
				return false;
		}
	}
#endif

	bool res = uart_tx(uart, buf, len, AT_UART_TX_WAIT_MS);

#if defined(MODEM_EMULATED_CTS) && defined(MODEM_EMULATED_RTS)
	if (m_rts != NC && m_cts != NC)
		gpio_write(m_rts, PIN_HIGH);
#endif

	return res;
}

void at_core_clear_rx(void)
{
	uart_util_flush();
}

/*
 * Generic utility to send command, wait for the response, and process response.
 * It is best suited for responses bound by delimiters with the exceptions
 * for write prompt command for tcp write, echo_off and modem_ok commands
 */
at_ret_code at_core_wcmd(const at_command_desc *desc, bool read_line)
{
	CHECK_NULL(desc, AT_FAILURE);

	at_ret_code result = AT_SUCCESS;
	char *comm;
	uint32_t timeout;
	buf_sz read_bytes;
	uint16_t wanted;
	/*
	 * Temporary wanted bytes and buffer needed when reading method is not
	 * by the line, maximum is 3 as write prompt response is \r\n@ or \r\n>
	 */
	uint8_t tmp_want = 0;
	char temp_buf[4];

	comm = desc->comm;
	timeout = desc->comm_timeout;

	DEBUG_V0("%s: sending %s\n", __func__, comm);
	result = __at_comm_send_and_wait_rsp(comm, strlen(comm),
			&timeout);
	if (result != AT_SUCCESS)
		goto done;

	state.proc_rsp = true;
	for(uint8_t i = 0; i < ARRAY_SIZE(desc->rsp_desc); i++) {
		if (!desc->rsp_desc[i].rsp)
			continue;

		size_t comm_sz = strlen(comm);
		size_t rsp_sz = strlen(desc->rsp_desc[i].rsp);
		size_t exrsp_sz = rsp_sz + comm_sz;
		char exrsp[comm_sz + rsp_sz + 1];
		memset(exrsp, 0, comm_sz + rsp_sz + 1);
		strncpy(exrsp, comm, comm_sz);
		strncat(exrsp, desc->rsp_desc[i].rsp, rsp_sz);

		if (read_line) {
			read_bytes = uart_util_line_avail(rsp_header, rsp_trailer);
			/*
			 * wait till we get whole line or time runs out
			 */
			while (read_bytes == 0) {
				result = __at_wait_for_rsp(&timeout);
				if (result == AT_RSP_TIMEOUT) {
					DEBUG_V0("%s: timed out for rsp: %s\n",
							__func__,
							desc->rsp_desc[i].rsp);
					DEBUG_V0("%s: dumping buffer\n",
							__func__);
					__at_dump_buffer(NULL, 0);
					goto done;
				}
				read_bytes = uart_util_line_avail(rsp_header,
						rsp_trailer);
			}
			wanted = 0;
			tmp_want = 0;
		} else {
			/* First three bytes to determine response */
			tmp_want = 3;
			memset(temp_buf, 0, tmp_want);
			result = __at_uart_waited_read(temp_buf, tmp_want,
					&timeout);
			if (result != AT_SUCCESS)
				break;
			if (strncmp(temp_buf, desc->rsp_desc[i].rsp, tmp_want) == 0)
				wanted = strlen(desc->rsp_desc[i].rsp) - tmp_want;
			else if(strncmp(temp_buf, exrsp, tmp_want) == 0)
				wanted = exrsp_sz - tmp_want;
			else {
				if (strncmp(temp_buf, err_str, tmp_want) == 0)
					wanted = strlen(err_str) - tmp_want;
				else if (desc->err) {
					if (strncmp(temp_buf, desc->err,
								tmp_want) == 0)
						wanted = strlen(desc->err) -
							tmp_want;
					else
						result = AT_WRONG_RSP;
				} else
					result = AT_WRONG_RSP;
			}
			if (result == AT_WRONG_RSP) {
				DEBUG_V0("%s: wrong response for command:%s\n",
						__func__, comm);
				sys_delay(RSP_BUF_DELAY);
				__at_dump_buffer(NULL, 0);
				break;
			}
			buf_sz rb = uart_util_available();
			result = __at_wait_for_bytes(&rb, wanted,
					&timeout);
			state.waiting_resp = false;
			if (result != AT_SUCCESS)
				break;
			read_bytes = rb + tmp_want;
		}

		/* start processing response */
		char rsp_buf[read_bytes + 1];
		memset(rsp_buf, 0, read_bytes + 1);
		if (tmp_want > 0) {
			if (tmp_want <= read_bytes)
				strncpy(rsp_buf, temp_buf, tmp_want);
			else {
				DEBUG_V0("%s: Unlikey tmp want value\n",
						__func__);
				result = AT_FAILURE;
				break;
			}
		}

		int rd_b = uart_util_read((uint8_t *)rsp_buf + tmp_want,
				read_bytes - tmp_want);
		if (rd_b == (read_bytes - tmp_want)) {
			if (strncmp(rsp_buf, desc->rsp_desc[i].rsp, rsp_sz) != 0
			&& strncmp(rsp_buf, exrsp, exrsp_sz) != 0) {
				DEBUG_V0("%s: Error rsp for command:%s\n",
						__func__, comm);
				result = __at_handle_error_rsp(rsp_buf,
						read_bytes,
						desc->rsp_desc[i].rsp,
						desc->err);
				break;
			} else {
				if (desc->rsp_desc[i].rsp_handler)
					desc->rsp_desc[i].rsp_handler(rsp_buf,
							read_bytes,
							desc->rsp_desc[i].rsp,
							desc->rsp_desc[i].data);
				result = AT_SUCCESS;
			}
		} else {
			DEBUG_V0("%s: uart read failed (unlikely) for"
					" command:%s\n", __func__,desc->comm);
			result = AT_FAILURE;
			break;
		}

	} /* for loop ends */
done:
	state.proc_rsp = false;
	state.waiting_resp = false;

	/* Wait for a given amount of time before executing next command */
	sys_delay(at_comm_delay_ms);

	/* check to see if we have urcs while command was executing
	 * if result was wrong response, chances are that we are out of sync
	 */
	DEBUG_V1("%s: Processing URCS outside call back\n", __func__);
	at_core_cleanup();
	DEBUG_V1("%s: result: %d\n", __func__, result);
	return result;
}

void at_core_process_urc(bool mode)
{
	if (mode)
		state.proc_urc = true;
	while (1) {
		uint16_t read_bytes = uart_util_line_avail(rsp_header, rsp_trailer);
		if (read_bytes == 0)
			break;
		char urc[read_bytes + 1];
		urc[read_bytes] = 0x0;
		if (uart_util_read((uint8_t *)urc, read_bytes) < 0) {
			DEBUG_V0("%s: read err (Unlikely)\n", __func__);
			continue;
		}
		DEBUG_V0("%s: looking to process urc: %s\n", __func__, urc);

		/* Process the network / modem specific URCs first */
		if (at_modem_process_urc(urc))
			continue;

		/* Process communication URCs next */
		urc_callback(urc);

		/* XXX: Unprocessed URCs arrive here */
	}
	if (mode)
		state.proc_urc = false;
}

at_ret_code at_core_modem_reset(void)
{
	if (!at_modem_sw_reset()) {
		DEBUG_V0("%s: Trying hardware reset\n", __func__);
		at_modem_hw_reset();
	}

	return at_modem_configure() ? AT_SUCCESS : AT_FAILURE;
}

bool at_core_is_proc_urc(void)
{
	return state.proc_urc;
}

bool at_core_is_proc_rsp(void)
{
	return state.proc_rsp;
}

static void at_core_uart_rx_callback(callback_event ev)
{
	switch (ev) {
	case UART_EVENT_RECVD_BYTES:
		if (state.waiting_resp) {
			DEBUG_V1("%s: got response\n", __func__);
			process_rsp = true;
		} else {
			serial_rx_callback();
		}
		break;
	case UART_EVENT_RX_OVERFLOW:
		DEBUG_V0("%s: rx overflows\n", __func__);
		at_core_clear_rx();
		break;
	default:
		DEBUG_V0("%s: Unknown event code: %d\n", __func__, ev);
		break;
	}
}

bool at_core_init(at_rx_callback rx_cb, at_urc_callback urc_cb, uint32_t d_ms)
{
	CHECK_NULL(rx_cb, false);
	serial_rx_callback = rx_cb;
	CHECK_NULL(urc_cb, false);
	urc_callback = urc_cb;

#if defined(MODEM_EMULATED_CTS) && defined(MODEM_EMULATED_RTS)
	if (!emu_hwflctrl(MODEM_EMULATED_RTS, MODEM_EMULATED_CTS))
		return false;
#endif
	at_comm_delay_ms = d_ms;
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
		.priority = MODEM_UART_IRQ_PRIORITY,
		.irq = true
	};
	uart = uart_init(&pins, &config);
	if (uart == NO_PERIPH)
		return false;
	bool res = uart_util_init(uart, IDLE_CHARS);
	CHECK_SUCCESS(res, true, false);

	uart_util_reg_callback(at_core_uart_rx_callback);
	process_rsp = false;
	at_core_clear_rx();

	if (!at_modem_init())
		return false;
	return true;
}

int at_core_find_pattern(int start_idx, const uint8_t *pattern, buf_sz nlen)
{
	return uart_util_find_pattern(start_idx, pattern, nlen);
}

buf_sz at_core_rx_available(void)
{
	return uart_util_available();
}

int at_core_read(uint8_t *buf, buf_sz sz)
{
	return uart_util_read(buf, sz);
}
