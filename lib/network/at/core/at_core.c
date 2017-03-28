/* Copyright (c) 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdio.h>
#include "platform.h"
#include "at_core.h"

#ifdef MODEM_TOBY201
#define MODEM_RESET_DELAY		45000 /* In milli seconds */
#define RESET_PULSE_WIDTH_MS		2100  /* Toby-L2 data sheet Section 4.2.9 */
#define AT_UART_TX_WAIT_MS		10000
#define IDLE_CHARS			10

/*
 * Waiting arbitrarily as we do not know for how many
 * bytes we will be waiting for when modem does not send wrong response in
 * totality
 */
#define RSP_BUF_DELAY			2000 /* In mili seconds */
#endif

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

enum modem_core_commands {
	MODEM_OK,	/* Simple modem check command, i.e. AT */
	ECHO_OFF,	/* Turn off echoing AT commands given to the modem */
	CME_CONF,	/* Change the error output format to numerical */
	MODEM_RESET,	/* Command to reset the modem */
	SIM_READY,	/* Check if the SIM is present */
	EPS_REG_QUERY,	/* Query EPS registration */
	EPS_URC_SET,	/* Set the EPS registration URC */
	ExPS_REG_QUERY,	/* Query extended packet switched network registration */
	ExPS_URC_SET,	/* Set the extended packet switched network reg URC */
	MODEM_CORE_END	/* End-of-commands marker */
};

typedef enum at_core_urc {
	EPS_STAT_URC,
	ExPS_STAT_URC,
	NUM_URCS
} at_core_urc;

static const char *at_urcs[NUM_URCS] = {
	[EPS_STAT_URC] = "\r\n+CEREG: ",
	[ExPS_STAT_URC] = "\r\n+UREG: "
};

static const at_command_desc modem_core[MODEM_CORE_END] = {
        [MODEM_OK] = {
                .comm = "at\r",
                .rsp_desc = {
                        {
                                .rsp = "at\r\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 100
        },
        [ECHO_OFF] = {
                .comm = "ate0\r",
                .rsp_desc = {
                        {
                                .rsp = "ate0\r\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\nERROR\r\n",
                .comm_timeout = 100
        },
        [CME_CONF] = {
                .comm = "at+cmee=1\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 100
        },
        [SIM_READY] = {
                .comm = "at+cpin?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+CPIN: READY\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 15000
        },
        [EPS_REG_QUERY] = {
                .comm = "at+cereg?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+CEREG: 1,1\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 100
        },
        [EPS_URC_SET] = {
                .comm = "at+cereg=1\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 100
        },
        [ExPS_REG_QUERY] = {
                .comm = "at+ureg?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+UREG: 1,7\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 100
        },
        [ExPS_URC_SET] = {
                .comm = "at+ureg=1\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 100
        },
        [MODEM_RESET] = {
                /*
		 * Response will be processed in __at_modem_reset_comm
                 * function
                 */
                .comm = "at+cfun=16\r",
                .err = NULL,
                .comm_timeout = 5000
        }
};

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
	uint32_t start = platform_get_tick_ms();
	uint32_t end = start;
	while (!process_rsp) {
		end = platform_get_tick_ms();
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
		*rcv_bytes = uart_rx_available();

	}
	state.waiting_resp = true;
	if (*timeout == 0) {
		*rcv_bytes = uart_rx_available();
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
        buf_sz rcvd = uart_rx_available();
        if (__at_wait_for_bytes(&rcvd, wanted, timeout) != AT_SUCCESS) {
                DEBUG_V0("%s: bytes not present\n", __func__);
                return AT_FAILURE;
        }

        if (uart_read((uint8_t *)buf, wanted) < wanted) {
                DEBUG_V0("%s: Unlikely read error\n", __func__);
                return AT_FAILURE;
        }
        return AT_SUCCESS;
}

bool at_core_write(uint8_t *buf, uint16_t len)
{
	return uart_tx(buf, len, AT_UART_TX_WAIT_MS);
}

void at_core_clear_rx(void)
{
	uart_flush_rx_buffer();
}

static void __at_dump_buffer(const char *buf, buf_sz len)
{
        if (!buf) {
                buf_sz bytes = uart_rx_available();
                if (bytes > 0) {
                        uint8_t temp_buf[bytes];
                        uart_read(temp_buf, bytes);
                        for (uint8_t i = 0; i < bytes; i++)
                                DEBUG_V0("0x%x, ", temp_buf[i]);
                }
        } else {
                for (uint8_t i = 0; i < len; i++)
                        DEBUG_V0("0x%x, ", buf[i]);
        }
        DEBUG_V0("\n");
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
	 * by the line, maximum is 3 as write prompt response is \r\n@
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
		if (read_line) {
			read_bytes = uart_line_avail(rsp_header, rsp_trailer);
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
				read_bytes = uart_line_avail(rsp_header,
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
			if (strncmp(temp_buf, desc->rsp_desc[i].rsp,
						tmp_want) == 0)
				wanted = strlen(desc->rsp_desc[i].rsp) - tmp_want;

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
				platform_delay(RSP_BUF_DELAY);
				__at_dump_buffer(NULL, 0);
				break;
			}
			buf_sz rb = uart_rx_available();
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

		int rd_b = uart_read((uint8_t *)rsp_buf + tmp_want,
				read_bytes - tmp_want);
		if (rd_b == (read_bytes - tmp_want)) {
			if (strncmp(rsp_buf, desc->rsp_desc[i].rsp,
						strlen(desc->rsp_desc[i].rsp))
					!= 0) {
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

	/* Recommeded to wait at least 20ms before proceeding */
	platform_delay(AT_COMM_DELAY_MS);

	/* check to see if we have urcs while command was executing
	 * if result was wrong response, chances are that we are out of sync
	 */
	DEBUG_V1("%s: Processing URCS outside call back\n", __func__);
	at_core_cleanup();
	DEBUG_V1("%s: result: %d\n", __func__, result);
	return result;
}

static at_ret_code __at_process_netw_urc(const char *urc, at_core_urc u_code)
{
	if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
		return AT_FAILURE;

	uint8_t last_char = strlen(at_urcs[u_code]);
	uint8_t net_stat = urc[last_char] - '0';
	DEBUG_V0("%s: Net stat (%u): %u\n", __func__, u_code, net_stat);
	switch (u_code) {
	case EPS_STAT_URC:
		if (net_stat == 0 || net_stat == 3 || net_stat == 4)
			DEBUG_V0("%s: EPS network reg lost\n", __func__);
		else
			DEBUG_V0("%s: Registered to EPS network\n", __func__);
		break;
	case ExPS_STAT_URC:
		if (net_stat == 0)
			DEBUG_V0("%s: Extended PS network reg lost\n", __func__);
		else
			DEBUG_V0("%s: Registered to extended PS network\n", __func__);
		break;
	default:
		return AT_FAILURE;
	}
	return AT_SUCCESS;
}

void at_core_process_urc(bool mode)
{
	if (mode)
		state.proc_urc = true;
	while (1) {
		uint16_t read_bytes = uart_line_avail(rsp_header, rsp_trailer);
		if (read_bytes == 0)
			break;
		char urc[read_bytes + 1];
		urc[read_bytes] = 0x0;
		if (uart_read((uint8_t *)urc, read_bytes) < 0) {
			DEBUG_V0("%s: read err (Unlikely)\n", __func__);
			continue;
		}
		DEBUG_V0("%s: looking to process urc: %s\n", __func__, urc);

		/* Process the network URCs within the core module */
		if (__at_process_netw_urc(urc, EPS_STAT_URC) == AT_SUCCESS)
			continue;

		if (__at_process_netw_urc(urc, ExPS_STAT_URC) == AT_SUCCESS)
			continue;

		urc_callback(urc);
	}
	if (mode)
		state.proc_urc = false;
}

/*
 * Resetting modem is a special case where its response depends on the previous
 * setting of the echo in the modem, where if echo is on it sends
 * command plus OK or OK otherwise as a response.
 */
static at_ret_code __at_modem_reset_comm(void)
{
	at_ret_code result = AT_FAILURE;

	const at_command_desc *desc = &modem_core[MODEM_RESET];
	char *comm = desc->comm;
	uint32_t timeout = desc->comm_timeout;
	uint16_t wanted;
	char *rsp = "\r\nOK\r\n";
	char alt_rsp[strlen(comm) + strlen(rsp) + 1];
	char *temp_rsp = NULL;
	strcpy(alt_rsp, comm);
	strcat(alt_rsp, rsp);

	int max_bytes = (strlen(rsp) > strlen(alt_rsp)) ?
		strlen(rsp):strlen(alt_rsp);

	char rsp_buf[max_bytes + 1];
	rsp_buf[max_bytes] = 0x0;

	result = __at_comm_send_and_wait_rsp(comm, strlen(comm), &timeout);
	if (result != AT_SUCCESS)
		goto done;

	state.proc_rsp = true;
	/* read for /r/nO or at+ */
	wanted = 3;
	result = __at_uart_waited_read(rsp_buf, wanted, &timeout);
	if (result != AT_SUCCESS)
		goto done;

	if (strncmp(rsp_buf, rsp, wanted) == 0) {
		temp_rsp = rsp + wanted;
		wanted = strlen(rsp) - wanted;
	} else if (strncmp(rsp_buf, alt_rsp, wanted) == 0) {
		temp_rsp = alt_rsp + wanted;
		wanted = strlen(alt_rsp) - wanted;
	} else {
		result = AT_FAILURE;
		DEBUG_V0("%s: wrong resp\n", __func__);
		platform_delay(RSP_BUF_DELAY);
		__at_dump_buffer(NULL, 0);
		goto done;
	}

	result = __at_uart_waited_read(rsp_buf, wanted, &timeout);
	if (result != AT_SUCCESS)
		goto done;
	if (strncmp(rsp_buf, temp_rsp, wanted) != 0) {
		DEBUG_V0("%s: Unlikely comparison error\n", __func__);
		result = AT_FAILURE;
	} else
		result = AT_SUCCESS;

done:
	state.waiting_resp = false;
	state.proc_rsp = false;
	platform_delay(AT_COMM_DELAY_MS);
	/* check to see if we have urcs while command was executing
	*/
	DEBUG_V1("%s: Processing URCS outside call back\n", __func__);
	at_core_cleanup();
	return result;
}

at_ret_code at_core_modem_reset(void)
{
	at_ret_code result = __at_modem_reset_comm();
#if 1
	if (result != AT_SUCCESS) {
		DEBUG_V0("%s: Trying hardware reset:%u\n", __func__, result);
		//platform_reset_modem(RESET_PULSE_WIDTH_MS);
	}
#endif
	/* sending at command right after reset command succeeds which is not
	 * desirable, wait here for few seconds before we send at command to
	 * poll for modem
	 */
	platform_delay(3000);
	uint32_t start = platform_get_tick_ms();
	uint32_t end;
	result = AT_FAILURE;
	while (result != AT_SUCCESS) {
		end = platform_get_tick_ms();
		if ((end - start) > MODEM_RESET_DELAY) {
			DEBUG_V0("%s: timed out\n", __func__);
			return result;
		}
		result =  at_core_wcmd(&modem_core[MODEM_OK], false);
		platform_delay(CHECK_MODEM_DELAY);
	}

	/* Switch off TX echo */
	result = at_core_wcmd(&modem_core[ECHO_OFF], false);
	CHECK_SUCCESS(result, AT_SUCCESS, result);

	/* Check if the SIM card is present */
	result = at_core_wcmd(&modem_core[SIM_READY], true);
	CHECK_SUCCESS(result, AT_SUCCESS, result);

	/* Set error format to numerical */
	result = at_core_wcmd(&modem_core[CME_CONF], true);
	CHECK_SUCCESS(result, AT_SUCCESS, result);

	/* Enable the Extended Packet Switched network registration URC */
	result = at_core_wcmd(&modem_core[ExPS_URC_SET], true);
	CHECK_SUCCESS(result, AT_SUCCESS, result);

	/* Enable the EPS network registration URC */
	result = at_core_wcmd(&modem_core[EPS_URC_SET], true);

	return result;
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

bool at_core_init(at_rx_callback rx_cb, at_urc_callback urc_cb)
{
	CHECK_NULL(rx_cb, false);
	serial_rx_callback = rx_cb;
	CHECK_NULL(urc_cb, false);
	urc_callback = urc_cb;

	bool res = uart_module_init(UART_EN_HW_CTRL, IDLE_CHARS);
	CHECK_SUCCESS(res, true, false);
	uart_set_rx_callback(at_core_uart_rx_callback);
	process_rsp = false;
	at_core_clear_rx();
	return true;
}

int at_core_find_pattern(int start_idx, const uint8_t *pattern, buf_sz nlen)
{
	return uart_find_pattern(start_idx, pattern, nlen);
}

buf_sz at_core_rx_available(void)
{
	return uart_rx_available();
}

int at_core_read(uint8_t *buf, buf_sz sz)
{
	return uart_read(buf, sz);
}

bool at_core_query_netw_reg(void)
{
	at_ret_code res = at_core_wcmd(&modem_core[EPS_REG_QUERY], true);
	if (res != AT_SUCCESS)
		return false;

	res = at_core_wcmd(&modem_core[ExPS_REG_QUERY], true);
	if (res != AT_SUCCESS)
		return false;

	return true;
}
