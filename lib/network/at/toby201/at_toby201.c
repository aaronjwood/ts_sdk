/*
 *  \file at_toby201.c, implementing APIs defined in at.h file
 *
 *  \brief AT command functions for u-blox toby201 lte modem
 *
 *  \copyright Copyright (C) 2016, 2017 Verizon. All rights reserved.
 *
 */

#include <errno.h>
#include <stm32f4xx_hal.h>
#include "at.h"
#include "at_toby201_command.h"
#include "uart.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "platform.h"

static const char *rsp_header = "\r\n";
static const char *rsp_trailer = "\r\n";

/* TCP operation can fail at tcp level as well operation level which can be
 * caught with +CME ERROR, while tcp level error requires issuing
 * special command to retrieve error number
 */
static const char *tcp_error = "\r\nERROR\r\n";

/* structure to hold dl mode related information */
static volatile struct {
        /* dl mode escape indication string from modem */
	const char *dis_str;
        buf_sz l_matched_pos;
        uint8_t matched_bytes;
        dis_states dis_state;
        at_intr_buf dl_buf;
} dl;

static volatile at_states state;

static volatile bool process_rsp;
/* Flag to indicate one time packet data network enable procedure */
static volatile bool pdp_conf;

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

static void __at_uart_rx_flush(void) {
        uart_flush_rx_buffer();
}

static bool __at_uart_write(uint8_t *buf, uint16_t len) {
        return uart_tx(buf, len, AT_UART_TX_WAIT_MS);
}

static at_ret_code __at_process_network_urc(char *urc, at_urc u_code)
{

        if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
                return AT_FAILURE;

        uint8_t count = strlen(at_urcs[u_code]);
        uint8_t net_stat = urc[count] - '0';
        DEBUG_V0("%s: net stat (%u): %u\n", __func__, u_code, net_stat);
        if (((u_code == NET_STAT_URC) && (net_stat != 1)) ||
                ((u_code == EPS_STAT_URC) && (net_stat == 0))) {
                state |= NETWORK_LOST;
                DEBUG_V0("%s: network lost\n", __func__);
        } else {
                state &= ~NETWORK_LOST;
                DEBUG_V0("%s: network restored\n", __func__);
        }
        return AT_SUCCESS;
}

static at_ret_code __at_process_dl_close_urc(char *urc, at_urc u_code)
{
        if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) == 0) {
                state &= ~DL_MODE;
                DEBUG_V0("%s:%d: direct link mode closed\n", __func__, __LINE__);
                return AT_SUCCESS;
        } else
                return AT_FAILURE;
}

static at_ret_code __at_process_pdp_tcp_close_urc(char *urc, at_urc u_code)
{
        if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) == 0) {
                switch (u_code) {
                case TCP_CLOSED:
                        state = TCP_REMOTE_DISCONN;
                        DEBUG_V0("%s: tcp closed\n", __func__);
                        return AT_SUCCESS;
                case PDP_DEACT:
                        DEBUG_V0("%s: pdp closed\n", __func__);
                        pdp_conf = false;
                        return AT_SUCCESS;
                }

        } else
                return AT_FAILURE;
}

static void __at_process_urc(void)
{

        while (1) {
                at_ret_code result = AT_SUCCESS;
                uint16_t read_bytes = uart_line_avail(rsp_header, rsp_trailer);
                if (read_bytes == 0)
                        break;
                uint8_t urc[read_bytes + 1];
                urc[read_bytes] = 0x0;
                if (uart_read(urc, read_bytes) < 0) {
                        DEBUG_V0("%s: read err (Unlikely)\n", __func__);
                        continue;
                }
                DEBUG_V0("%s: looking to process urc: %s\n",
                                                __func__, (char *)urc);

                result = __at_process_network_urc((char *)urc, NET_STAT_URC);
                if (result == AT_SUCCESS)
                        continue;

                result = __at_process_network_urc((char *)urc, EPS_STAT_URC);
                if (result == AT_SUCCESS)
                        continue;

                result = __at_process_pdp_tcp_close_urc((char *)urc, TCP_CLOSED);
                if (result == AT_SUCCESS)
                        continue;
                result = __at_process_dl_close_urc((char *)urc, DISCONNECT);
                if (result == AT_SUCCESS)
                        continue;
                result = __at_process_pdp_tcp_close_urc((char *)urc, PDP_DEACT);
        }
}

static void __at_lookup_dl_esc_str(void)
{
        int nw_f;
        int cur_f;
        uint8_t itr = 0;
        for (; itr < strlen(dl.dis_str); itr++) {
                nw_f = uart_find_pattern(-1, dl.dis_str, itr + 1);
                if (nw_f == -1) {
                        if (dl.dis_state == DIS_IDLE) {
                                if (itr > 0) {
                                        /* this may happen when string
                                         * truely is partially received and
                                         * buffer is scanned completely
                                         */
                                        dl.dis_state = DIS_PARTIAL;
                                        dl.matched_bytes = itr;
                                        dl.l_matched_pos = cur_f;
                                }
                        } else if (dl.dis_state == DIS_PARTIAL) {
                                if (itr == 0) {
                                        /* this condition is possible if tcp
                                         * rcv happend before this
                                         * and new chunk does not match
                                         * in that case back to reset
                                         */
                                         dl.dis_state = DIS_IDLE;
                                         dl.matched_bytes = 0;
                                         dl.l_matched_pos = -1;
                                } else {
                                        /* this is possible when partial pattern
                                         * just happens to be in the binary data
                                         * and has not received escape pattern
                                         * yet
                                         * reset everything and start over for
                                         * the new chunk of data arrives
                                         *
                                         * there is no possibility that it may
                                         * find same number of bytes somewhere
                                         * else since pattern finding logic just
                                         * returns as soon as it finds position
                                         */
                                        if (dl.matched_bytes == itr) {
                                                if (dl.l_matched_pos == cur_f) {
                                                        dl.dis_state = DIS_IDLE;
                                                        dl.matched_bytes = 0;
                                                        dl.l_matched_pos = -1;
                                                } else
                                                        DEBUG_V0("%d:UERR#@$\n",
                                                                __LINE__);

                                        } else {
                                        /* new chunk received has matched new
                                         * set of bytes and may be new
                                         * position, itr can be greater or less
                                         * from previously matched bytes based
                                         * if tcp rcv call happened before this
                                         * new arrival
                                         */
                                                dl.matched_bytes = itr;
                                                dl.l_matched_pos = cur_f;
                                        }
                                }
                        } else
                                DEBUG_V0("%d:UERR\n", __LINE__);
                        return;
                } else
                        cur_f = nw_f;
        }
        dl.dis_state = DIS_FULL;
        dl.matched_bytes = 0;
        dl.l_matched_pos = -1;
}

/* Function will be called when full disconnect string is detected when remote
 * side closes connection abruptly
 * It will transfer binary data into AT layer intermediate buffer till
 * end of the disconnect string by comparing every character it reads to the
 * hardcoded disconnect string, after that it will process all remaining bytes
 * from the uart buffer as it may contain URCs while in a DL mode
 * For example: UART buffer possible state as below:
 *		[<binary data>,<disconnect string>,<URCs>]
 *		Function will transfer "binary data " to intermediate buffer
 *		Optionally can proces "URCs" if present
 */

static void __at_xfer_to_buf()
{
	dl.dl_buf.ridx = 0;
        buf_sz sz = uart_rx_available();
        if (sz == 0) {
                DEBUG_V0("%d:UERR\n", __LINE__);
                dl.dl_buf.buf_unread = 0;
                return;
        }
        DEBUG_V0("%d:SZ_A:%d\n", __LINE__, sz);
        /* 3 is to account for trailing x\r\n, where x is socket id */
        if ((sz - (strlen(dl.dis_str) + 3)) == 0)
                __at_uart_rx_flush();
        else {
		buf_sz start_idx = 0;
		/* Index to keep track matched characters from dl.dis_str string
		*/
		buf_sz match_idx = 0;
		/* Index to starting position of the matched string, we need
		 * this to use it as a size of binary data as from found_idx till
		 * strlen(dl.dis_str) + 3 will be disconnect string and will be
		 * ignored
		 */
		buf_sz found_idx = 0;
		bool found_start = false;
		/* This will be set to reset match_idx when partial disconnect
		 * string (can be binary data) and actual disconnect string are
		 * adjacent, for example: "matcmatched", where "matched" is the
		 * string we are looking for and "matc" just happens to be binary
		 * data placed togather
		 */
		bool skip_read = false;
		while (1) {
			if (start_idx >= sz)
				break;
			if (!skip_read) {
				if (uart_read((uint8_t *)dl.dl_buf.buf +
							start_idx, 1) != 1)
					break;
			}
			if (dl.dis_str[match_idx] == dl.dl_buf.buf[start_idx]) {
				if (!found_start) {
					found_start = true;
					found_idx = start_idx;
				}
				match_idx++;
				start_idx++;
				skip_read = false;
			} else {
				if (found_start) {
					found_start = false;
					skip_read = true;
				} else {
					start_idx++;
					skip_read = false;
				}
				found_idx = 0;
				match_idx = 0;
			}
			if (found_start && (match_idx == strlen(dl.dis_str)))
				break;
		}

                if (!found_start || (found_idx == 0)) {
			DEBUG_V0("%d:UERR\n", __LINE__);
			goto done;
		}
		/* we are only interested in data till found_idx as after that it
		 * will be dl.dl_str string which we don't care
		 */
                dl.dl_buf.buf_unread = found_idx;
		/* 3 is to account for trailing x\r\n, where x is socket id */
		uint8_t temp_buf[3];
		if (uart_read(temp_buf, 3) != 3) {
			DEBUG_V0("%d:UERR\n", __LINE__);
			goto done;
		}
		/* Since we processed all the disconnect string, there may be
		 * other URCs buffered in the modem while we were in DL Mode
		 */
		state |= PROC_URC;
	        __at_process_urc();
	        state &= ~PROC_URC;

		return;
done:
		dl.dl_buf.buf_unread = 0;
		dl.dl_buf.ridx = 0;
		__at_uart_rx_flush();
		return;
        }
}

static void at_uart_callback(callback_event ev)
{
        switch (ev) {
        case UART_EVENT_RECVD_BYTES:
                if ((state & WAITING_RESP) == WAITING_RESP) {
                        DEBUG_V1("%s: got response\n", __func__);
                        process_rsp = true;
                } else if (((state & DL_MODE) == DL_MODE) &&
                        ((state & TCP_DL_RX) != TCP_DL_RX)) {
                        if (dl.dis_state < DIS_FULL) {
                                __at_lookup_dl_esc_str();
                                if (dl.dis_state == DIS_FULL) {
                                        state = TCP_REMOTE_DISCONN;
                                        __at_xfer_to_buf();
                                }
                        }
                } else if (((state & PROC_RSP) != PROC_RSP) &&
                        ((state & PROC_URC) != PROC_URC) &&
                        ((state & DL_MODE) != DL_MODE)) {
                                DEBUG_V1("%s: urc from callback:%d\n",
                                        __func__, state);
                                __at_process_urc();
                } else
                        DEBUG_V1("%s:%d:state:%d\n", __func__, __LINE__, state);
                break;
        case UART_EVENT_RX_OVERFLOW:
                DEBUG_V0("%s: rx overflows\n", __func__);
                __at_uart_rx_flush();
                break;
        default:
                DEBUG_V0("%s: Unknown event code: %d\n", __func__, ev);
                break;
        }

}

static void __at_cleanup(void)
{
        state |= PROC_URC;
        __at_process_urc();
        state &= ~PROC_URC;
        __at_uart_rx_flush();

}

static at_ret_code __at_wait_for_rsp(uint32_t *timeout)
{

        state |= WAITING_RESP;
        at_ret_code result = AT_SUCCESS;
        uint32_t start = platform_get_tick_ms();
        uint32_t end;
        while (!process_rsp) {
                end = platform_get_tick_ms();
                if ((end - start) > *timeout) {
                        result = AT_RSP_TIMEOUT;
                        DEBUG_V1("%s: RSP_TIMEOUT: out of %u, waited %u\n",
                                __func__, *timeout, (end - start));
                        *timeout = 0;
                        break;
                }
        }
        state &= ~(WAITING_RESP);
        process_rsp = false;
        if (*timeout != 0 && ((end - start) < *timeout))
                *timeout = *timeout - (end - start);
        return result;
}

static at_ret_code __at_comm_send_and_wait_rsp(char *comm, uint16_t len,
                                                uint32_t *timeout)
{
        /* Process urcs if any before we send down new command
         * and empty buffer
         */
        __at_cleanup();
        CHECK_NULL(comm, AT_FAILURE)

        if (!__at_uart_write((uint8_t *)comm, len)) {
                DEBUG_V0("%s: uart tx fail\n", __func__);
                return AT_TX_FAILURE;
        }
        return __at_wait_for_rsp(timeout);
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

static at_ret_code __at_handle_error_rsp(uint8_t *rsp_buf, buf_sz read_bytes,
                                        const char *rsp, const char *err)
{
        at_ret_code result = AT_SUCCESS;
        if (err) {
                if (strncmp((char *)rsp_buf, err, strlen(err)) != 0) {
                        if (strncmp((char *)rsp_buf, tcp_error,
                                strlen(tcp_error)) == 0) {
                                DEBUG_V0("%s: tcp error: %s\n",
                                        __func__, (char *)rsp_buf);
                                result = AT_TCP_FAILURE;
                        } else {
                                DEBUG_V0("%s: wrong rsp: %s\n",
                                        __func__, (char *)rsp_buf);
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

static at_ret_code __at_wait_for_bytes(buf_sz *rcv_bytes,
                                        buf_sz target_bytes,
                                        uint32_t *timeout)
{
        while ((*rcv_bytes < target_bytes) && (*timeout > 0)) {
                /* Modem may be busy and could not send whole response that
                 * we are looking for, wait here to give another chance
                 */
                __at_wait_for_rsp(timeout);
                state |= WAITING_RESP;
                /* New total bytes available to read */
                *rcv_bytes = uart_rx_available();

        }
        state |= WAITING_RESP;
        if (*timeout == 0) {
                *rcv_bytes = uart_rx_available();
                if (*rcv_bytes < target_bytes)
                        return AT_FAILURE;
        }
        DEBUG_V1("%s: data available (%u), wanted (%u), waited for %u time\n",
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
/* Generic utility to send command, wait for the response, and process response.
 * It is best suited for responses bound by delimiters with the exceptions
 * for write prompt command for tcp write, echo_off and modem_ok commands
 */

static at_ret_code __at_generic_comm_rsp_util(const at_command_desc *desc,
                                        bool skip_comm, bool read_line)
{
        CHECK_NULL(desc, AT_FAILURE)

        at_ret_code result = AT_SUCCESS;
        char *comm;
        const char *rsp;
        uint32_t timeout;
        buf_sz read_bytes;
        uint16_t wanted;
        /* temperary wanted bytes and buffer needed when reading method is not
         * by the line, maximum is 3 as write prompt response is \r\n@
         */
        uint8_t tmp_want = 0;
        char temp_buf[4];

        comm = desc->comm;
        timeout = desc->comm_timeout;

        if (!skip_comm) {
                DEBUG_V0("%s: sending %s\n", __func__, comm);
                result = __at_comm_send_and_wait_rsp(comm, strlen(comm),
                                                                &timeout);
                if (result != AT_SUCCESS)
                        goto done;
        }

        state |= PROC_RSP;
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
                                                __func__, desc->rsp_desc[i].rsp);
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
                                if (strncmp(temp_buf, tcp_error, tmp_want) == 0)
                                        wanted = strlen(tcp_error) - tmp_want;
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
                        state &= ~WAITING_RESP;
                        if (result != AT_SUCCESS)
                                break;
                        read_bytes = rb + tmp_want;
                }

                /* start processing response */
                uint8_t rsp_buf[read_bytes + 1];
                memset(rsp_buf, 0, read_bytes + 1);
                if (tmp_want > 0) {
                        if (tmp_want <= read_bytes)
                                strncpy((char *)rsp_buf, temp_buf, tmp_want);
                        else {
                                DEBUG_V0("%s: Unlikey tmp want value\n",
                                        __func__);
                                result = AT_FAILURE;
                                break;
                        }
                }

                int rd_b = uart_read(rsp_buf + tmp_want, read_bytes - tmp_want);
                if (rd_b == (read_bytes - tmp_want)) {
                        if (strncmp((char *)rsp_buf,
                                desc->rsp_desc[i].rsp,
                                strlen(desc->rsp_desc[i].rsp)) != 0) {
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
        state &= ~PROC_RSP;
        state &= ~WAITING_RESP;

        /* Recommeded to wait at least 20ms before proceeding */
        platform_delay(AT_COMM_DELAY_MS);

        /* check to see if we have urcs while command was executing
         * if result was wrong response, chances are that we are out of sync
         */
        DEBUG_V1("%s: Processing URCS outside call back\n", __func__);
        __at_cleanup();
        DEBUG_V1("%s: result: %d\n", __func__, result);
        return result;
}

/* resetting modem is a special case where its response depends on the previous
 * setting of the echo in the modem, where if echo is on it sends
 * command plus OK or OK otherwise as a response
 */
static at_ret_code __at_modem_reset_comm()
{
        at_ret_code result = AT_FAILURE;

        const at_command_desc *desc = &modem_net_status_comm[MODEM_RESET];
        char *comm = desc->comm;
        uint32_t timeout = desc->comm_timeout;
        uint16_t rcvd, wanted;
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

        state |= PROC_RSP;
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
        state &= ~WAITING_RESP;
        state &= ~PROC_RSP;
        platform_delay(AT_COMM_DELAY_MS);
        /* check to see if we have urcs while command was executing
         */
        DEBUG_V1("%s: Processing URCS outside call back\n", __func__);
        __at_cleanup();
        return result;
}

static at_ret_code __at_modem_reset()
{
        at_ret_code result;
        result = __at_modem_reset_comm();
        CHECK_SUCCESS(result, AT_SUCCESS, result)
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
                result =  __at_generic_comm_rsp_util(
                                &modem_net_status_comm[MODEM_OK], false, false);
                platform_delay(CHECK_MODEM_DELAY);
        }

        result = __at_generic_comm_rsp_util(
                                &modem_net_status_comm[ECHO_OFF], false, false);
        CHECK_SUCCESS(result, AT_SUCCESS, result)

        result = __at_generic_comm_rsp_util(
                                &modem_net_status_comm[CME_CONF], false, true);

        return result;
}

static at_ret_code __at_check_network_registration()
{
        at_ret_code result1 = AT_SUCCESS;
        at_ret_code result2 = AT_SUCCESS;
        /* Check network registration status */
        result1 = __at_generic_comm_rsp_util(
                        &modem_net_status_comm[NET_REG_STAT], false, true);

        /* Check packet switch network registration status */
        result2 = __at_generic_comm_rsp_util(
                        &modem_net_status_comm[EPS_REG_STAT], false, true);

        if ((result1 == AT_SUCCESS) && (result2 == AT_SUCCESS))
                return AT_SUCCESS;
        return AT_FAILURE;
}

static at_ret_code __at_modem_conf() {

        at_ret_code result = AT_SUCCESS;

        /* Enable EPS network status URCs */
        result = __at_generic_comm_rsp_util(&modem_net_status_comm[EPS_STAT],
                                                false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result)

        /* ENABLE network status URCs */
        result = __at_generic_comm_rsp_util(&modem_net_status_comm[NET_STAT],
                                                false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result)

        /* Check if simcard is inserted */
        result = __at_generic_comm_rsp_util(&modem_net_status_comm[SIM_READY],
                                                false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result)

        /* set escape sequence delay */
        char temp_comm[TEMP_COMM_LIMIT];
        at_command_desc *desc = &tcp_comm[ESCAPE_TIME_CONF];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, DL_MODE_ESC_TIME);
        desc->comm = temp_comm;
        result = __at_generic_comm_rsp_util(desc, false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result)

        /* Check MNO configuration, if it is not set for the Verizon, configure
         * for it and reset the modem to save settings
         */
        result = __at_generic_comm_rsp_util(&modem_net_status_comm[MNO_STAT],
                                                false, true);
        if (result != AT_SUCCESS) {
                result = __at_generic_comm_rsp_util(
                                &modem_net_status_comm[MNO_SET], false, true);
                CHECK_SUCCESS(result, AT_SUCCESS, result)
                uint8_t res = __at_modem_reset();
                if (res == AT_RSP_TIMEOUT || res == AT_TX_FAILURE)
                        return res;
                return AT_RECHECK_MODEM;
        }

        result = AT_FAILURE;
        uint32_t start = platform_get_tick_ms();
        uint32_t end;
        while (result != AT_SUCCESS) {
                end = platform_get_tick_ms();
                DEBUG_V0("%s: Rechecking network registration\n", __func__);
                if ((end - start) > NET_REG_CHECK_DELAY) {
                        DEBUG_V0("%s: timed out\n", __func__);
                        break;
                }
                result = __at_check_network_registration();
                platform_delay(CHECK_MODEM_DELAY);
        }

        /* Now modem has registered with home network, it is safe to say network
         * is ready for tcp connection
         */
        if (result == AT_SUCCESS)
                state &= ~NETWORK_LOST;
        else {
                DEBUG_V0("%s: modem is not connected to network\n", __func__);
                state |= NETWORK_LOST;
        }

        return result;
}

bool at_init()
{
        dl.dis_str = "\r\nDISCONNECT\r\n\r\nOK\r\n\r\n+UUSOCL: ";
        dl.l_matched_pos = -1;
        dl.matched_bytes = 0;
        dl.dl_buf.buf_unread = 0;
        dl.dl_buf.ridx= 0;
        dl.dis_state = DIS_IDLE;

        bool res = uart_module_init(UART_EN_HW_CTRL, IDLE_CHARS);
        CHECK_SUCCESS(res, true, false)

        uart_set_rx_callback(at_uart_callback);
        state = IDLE;
        process_rsp = false;
        pdp_conf = false;
        __at_uart_rx_flush();
        /* This may take few seconds */
        at_ret_code res_modem = __at_modem_reset();
        if (res_modem != AT_SUCCESS) {
                printf("%s: modem reset failed (%d)\n", __func__, res_modem);
                return false;
        }

        res_modem = __at_modem_conf();
        if (res_modem == AT_RECHECK_MODEM) {
                DEBUG_V1("%s: Rechecking modem config after reset\n", __func__);
                res_modem =  __at_modem_conf();
        }

        if (res_modem != AT_SUCCESS) {
                state = AT_INVALID;
                return false;
        }

        return true;

}

static void __at_reset_dl_state(void)
{
        dl.dis_state = DIS_IDLE;
        dl.dl_buf.buf_unread = 0;
        dl.dl_buf.ridx = 0;
        dl.l_matched_pos = -1;
        dl.matched_bytes = 0;
}

static at_ret_code __at_esc_dl_mode(void)
{
        platform_delay(DL_ESC_TIME_MS);
        at_command_desc *desc = &tcp_comm[ESCAPE_DL_MODE];
        at_ret_code result = __at_generic_comm_rsp_util(desc, false, true);
        if (AT_COMM_DELAY_MS < DL_ESC_TIME_MS)
                platform_delay(DL_ESC_TIME_MS - DL_ESC_TIME_MS);

        if (result != AT_SUCCESS)
                DEBUG_V0("%s:%d: unable to exit dl mode\n", __func__, __LINE__);
        else
                DEBUG_V0("%s:%d: exited from dl mode\n", __func__, __LINE__);
        state &= ~DL_MODE;
        return result;
}

static at_ret_code __at_set_dl_mode(int s_id)
{
        if ((state & TCP_CONNECTED) != TCP_CONNECTED)
                return AT_FAILURE;
        at_command_desc *desc = &tcp_comm[TCP_DL_MODE];
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id);

        desc->comm = temp_comm;
        at_ret_code result = __at_generic_comm_rsp_util(desc, false, true);
        if (result == AT_SUCCESS) {
                state |= DL_MODE;
                DEBUG_V0("%s:%d: direct mode enabled\n", __func__, __LINE__);
        } else
                state &= ~DL_MODE;

        return result;

}

static at_ret_code __at_conf_dl_mode(int s_id)
{
        at_command_desc *desc = &tcp_comm[DL_CONG_CONF];
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id);

        desc->comm = temp_comm;
        return __at_generic_comm_rsp_util(desc, false, true);
}

static void __at_parse_tcp_conf_rsp(void *rcv_rsp, int rcv_rsp_len,
                                const char *stored_rsp , void *data)
{
        if (!data)
                return;
        uint16_t count = strlen(stored_rsp);
        if (count > rcv_rsp_len) {
                DEBUG_V0("%s: stored rsp is bigger then received rsp\n",
                        __func__);
                *((int *)data) = -1;
                return;
        }
        int s_id = (*((char *)(rcv_rsp + count))) - '0';
        *((int *)data) = s_id;
        DEBUG_V1("%s: processed tcp config rsp: %d\n",
                                __func__, *((int *)data));
}

static int __at_tcp_connect(const char *host, const char *port)
{
        int s_id = -1;
        at_ret_code result = AT_SUCCESS;
        at_command_desc *desc = &tcp_comm[TCP_CONF];
        desc->rsp_desc[0].data = &s_id;

        /* Configure tcp connection first to be tcp client */
        result = __at_generic_comm_rsp_util(desc, false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, -1);
        if (s_id < 0) {
                DEBUG_V0("%s: could not get socket: %d\n", __func__, s_id);
                return AT_SOCKET_FAILED;
        }

        /* Now make remote connection */
        desc = &tcp_comm[TCP_CONN];
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id,
                host, port);

        desc->comm = temp_comm;
        result = __at_generic_comm_rsp_util(desc, false, true);

        CHECK_SUCCESS(result, AT_SUCCESS, AT_CONNECT_FAILED);

        state |= TCP_CONNECTED;
        state &= ~TCP_CONN_CLOSED;
        DEBUG_V0("%s: socket:%d created\n", __func__, s_id);
        return s_id;
}

static at_ret_code __at_pdp_conf(void)
{
        at_ret_code result = __at_generic_comm_rsp_util(
                                        &pdp_conf_comm[SEL_IPV4_PREF],
                                        false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result)
        return __at_generic_comm_rsp_util(&pdp_conf_comm[ACT_PDP], false, true);
}

int at_tcp_connect(const char *host, const char *port)
{

        CHECK_NULL(host, -1)
        if ((state & DL_MODE) == DL_MODE) {
                DEBUG_V0("%s: Direct link mode is already on, tcp connect not "
                        "possible, state :%u\n",__func__, state);
                return -1;
        }
        if (state > IDLE) {
                if ((state & TCP_CONN_CLOSED) != TCP_CONN_CLOSED) {
                        DEBUG_V0("%s: TCP connect not possible, state :%u\n",
                                __func__, state);
                        return -1;
                }
        }
        if (!pdp_conf) {
                if (__at_pdp_conf() != AT_SUCCESS) {
                        DEBUG_V0("%s: PDP configuration failed\n", __func__);
                        return -1;
                }
                pdp_conf = true;

        }
        int s_id = __at_tcp_connect(host, port);
        if (s_id >= 0) {
                at_ret_code result1 = __at_conf_dl_mode(s_id);
                at_ret_code result2 = __at_set_dl_mode(s_id);
                if ((result1 != AT_SUCCESS) || (result2 != AT_SUCCESS)) {
                        DEBUG_V0("%s:%d: setting dl mode failed\n",
                                        __func__, __LINE__);
                        at_tcp_close(s_id);
                        return AT_CONNECT_FAILED;
                }
        }
        __at_reset_dl_state();
        return s_id;
}

static int __at_tcp_tx(const uint8_t *buf, size_t len)
{
        for (int i = 0; i < len; i++) {
                if ((state & TCP_CONNECTED) == TCP_CONNECTED) {
                        if (!__at_uart_write(((uint8_t *)buf + i), 1))
                                return AT_TCP_SEND_FAIL;
                } else {
                        DEBUG_V0("%s: diconnected in middle of the write\n",
                                __func__);
                        state &= ~TCP_CONN_CLOSED;
                        return AT_TCP_CONNECT_DROPPED;
                }
        }
        return 0;
}

int at_tcp_send(int s_id, const unsigned char *buf, size_t len)
{
        CHECK_NULL(buf, AT_TCP_INVALID_PARA)
        if ((s_id < 0) || (len == 0))
                return AT_TCP_INVALID_PARA;

        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                if ((state & TCP_CONN_CLOSED) == TCP_CONN_CLOSED)
                        return 0;
                DEBUG_V0("%s: tcp not connected\n", __func__);
                return AT_TCP_SEND_FAIL;
        }

        if ((state & DL_MODE) == DL_MODE) {
                int result = __at_tcp_tx(buf, len);
                if (result != 0) {
                        DEBUG_V0("%s:%d: write failed\n", __func__, __LINE__);
                        return result;
                }
                DEBUG_V1("%s: data written in dl mode: %d\n", __func__, len);
                return len;
        } else {
                DEBUG_V0("%s:%d: data send failed (no dl mode)\n",
                        __func__, __LINE__);
                return AT_TCP_SEND_FAIL;
        }
}

int at_read_available(int s_id)
{

        (void)(s_id);
        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                if ((state & TCP_CONN_CLOSED) == TCP_CONN_CLOSED)
                        return 0;
                if (dl.dl_buf.buf_unread > 0)
                        return dl.dl_buf.buf_unread;
                DEBUG_V0("%s: tcp not connected to read\n", __func__);
                return AT_TCP_RCV_FAIL;
        }

        if ((state & DL_MODE) != DL_MODE) {
                DEBUG_V0("%s:%d: dl mode is off\n", __func__, __LINE__);
                return AT_TCP_RCV_FAIL;
        }
        return uart_rx_available();
}

static buf_sz __at_calc_new_len(void)
{
        buf_sz n_sz = uart_rx_available();
        if (dl.matched_bytes > n_sz) {
                DEBUG_V0("%s:%d: Unlkly error\n", __func__, __LINE__);
                __at_reset_dl_state();
                return DL_PARTIAL_ERROR;
        }
        buf_sz a_avail = n_sz - dl.matched_bytes;
        DEBUG_V0("%s:%d: New avail in partial mode:%u, matched bytes:%u\n",
                __func__, __LINE__, a_avail, dl.matched_bytes);
        return a_avail;
}

static int __at_process_intr_buffer(unsigned char *buf, size_t len)
{
        if (dl.dl_buf.buf_unread == 0) {
                dl.dl_buf.ridx = 0;
                return 0;
        }

        size_t read_b = (len < dl.dl_buf.buf_unread) ? len : dl.dl_buf.buf_unread;
        memcpy(buf, (uint8_t *)dl.dl_buf.buf + dl.dl_buf.ridx, read_b);
        dl.dl_buf.buf_unread = dl.dl_buf.buf_unread - read_b;
        dl.dl_buf.ridx += read_b;
        return read_b;
}

static int __at_test_dl_partial_state()
{
        if (dl.dis_state == DIS_PARTIAL) {
                DEBUG_V1("%s:%d: unchanged state\n",__func__, __LINE__);
                return DL_PARTIAL_SUC;
        }
        if (dl.dis_state == DIS_FULL) {
                DEBUG_V1("%s:%d: partial became full\n", __func__, __LINE__);
                return DL_PARTIAL_TO_FULL;
        }

        if (dl.dis_state == DIS_IDLE) {
                DEBUG_V1("%s:%d: fake partial detected\n",__func__, __LINE__);
                return DL_PARTIAL_ERROR;
        }
}

static int __at_process_rcv_dl_partial(size_t len)
{
        buf_sz prev_matched = dl.matched_bytes;
        buf_sz prev_pos = dl.l_matched_pos;
        buf_sz sz = uart_rx_available();

        if (prev_matched > sz) {
                DEBUG_V0("%s:%d: matched bytes (%u) greater then avail (%u)\n",
                        __func__, __LINE__, prev_matched, sz);
                __at_reset_dl_state();
                return DL_PARTIAL_ERROR;
        }
        buf_sz a_avail = sz - prev_matched;
        DEBUG_V0("%s:%d: In partial mode:%u,bytes matched:%u\n",
                __func__, __LINE__, a_avail, prev_matched);

        /* we have read it out all the bytes but the matched partial
         * response bytes, look for escape string again and check state machine
         * if it has not changed, give a last try wait for some time to check if
         * partial response is really a part of the dl escape sequence or just
         * binary blob happen to be residing at the bottom of buffer matching
         * partially
         */
        if (a_avail == 0) {
                DEBUG_V0("%s:%d: Only partial matched byte is available\n",
                        __func__, __LINE__);

                state |= TCP_DL_RX;
                __at_lookup_dl_esc_str();
                state &= ~TCP_DL_RX;

                int r = __at_test_dl_partial_state();
                if (r != DL_PARTIAL_SUC)
                        return r;

                DEBUG_V0("%s:%d: Waiting for more bytes if any\n",
                        __func__, __LINE__);

                platform_delay(DL_PARTIAL_WAIT);

                r = __at_test_dl_partial_state();
                if (r != DL_PARTIAL_SUC)
                        return r;

                /* chances that last bytes are binary remnants , check if it
                 * still partial state assuming during that delay new bytes have
                 * arrived
                 */
                if (prev_pos != dl.l_matched_pos) {
                        a_avail = __at_calc_new_len();
                        DEBUG_V0("%s:%d: new available: (%u)\n",
                                __func__, __LINE__, a_avail);
                        if (a_avail == DL_PARTIAL_ERROR)
                                return DL_PARTIAL_ERROR;
                } else {
                        if (prev_matched == dl.matched_bytes) {
                                /* no change in data, consider this as a
                                 * binary blob and reset dl escape mode
                                 * state machine
                                 */
                                a_avail = uart_rx_available();
                                __at_reset_dl_state();
                        } else if (dl.matched_bytes > prev_matched) {
                                DEBUG_V0("%s:%d: Escape response"
                                        " is still in progress\n",
                                        __func__, __LINE__);
                                errno = EAGAIN;
                                return AT_TCP_RCV_FAIL;
                        } else {
                                DEBUG_V0("%s:%d: Unlikely error\n",
                                        __func__, __LINE__);
                                __at_reset_dl_state();
                                return DL_PARTIAL_ERROR;
                        }

                }
        } /* available if ends */

        return a_avail;
}

int at_tcp_recv(int s_id, unsigned char *buf, size_t len)
{
        if (s_id < 0 || !buf) {
                DEBUG_V0("%s: socket or buffer invalid\n", __func__);
                return AT_TCP_INVALID_PARA;
        }
        (void)(s_id);
        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                if ((state & TCP_CONN_CLOSED) == TCP_CONN_CLOSED)
                        return 0;
                /* Look into intermediate buffer before returning an error
                 * possiblity that it may have received bytes before remote
                 * site closed connection
                 */
                int res = __at_process_intr_buffer(buf, len);
                if (res != 0)
                        return res;
                DEBUG_V0("%s: tcp not connected to recv\n", __func__);
                return AT_TCP_RCV_FAIL;
        }

        if ((state & DL_MODE) == DL_MODE) {
                if (dl.dis_state == DIS_PARTIAL) {

                        int res = __at_process_rcv_dl_partial(len);
                        if (res == DL_PARTIAL_ERROR)
                                len = uart_rx_available();
                        else if (res == DL_PARTIAL_TO_FULL)
                                return __at_process_intr_buffer(buf, len);
                        else if (res > DL_PARTIAL_ERROR) {
                                DEBUG_V1("%s:%d: request (%u), available (%u)\n",
                                        __func__, __LINE__, len, res);
                                if (len > res)
                                        len = res;
                        } else
                                DEBUG_V0("%s:%d: Unlikly Err\n",
                                        __func__, __LINE__);
                }
                int rdb = uart_read(buf, len);
                if (rdb == 0) {
                        DEBUG_V1("%s:%d: read again\n", __func__, __LINE__);
                        errno = EAGAIN;
                        return AT_TCP_RCV_FAIL;
                }
                DEBUG_V1("%s:%d: read:%d, wanted:%d in a dl\n",
                                __func__, __LINE__, rdb, len);
                return rdb;
        } else {
                DEBUG_V0("%s:%d: dl mode not on\n", __func__, __LINE__);
                return AT_TCP_RCV_FAIL;
        }
}

void at_tcp_close(int s_id)
{
        if (s_id < 0)
                return;

        dl.dl_buf.buf_unread = 0;
        dl.dl_buf.ridx = 0;

        if ((state & TCP_REMOTE_DISCONN) == TCP_REMOTE_DISCONN) {
                DEBUG_V0("%s:%d: tcp already closed\n", __func__, __LINE__);
                __at_reset_dl_state();
                __at_cleanup();
                state = IDLE;
                return;
        }

        if ((state & DL_MODE) == DL_MODE)
                __at_esc_dl_mode();


        at_command_desc *desc = &tcp_comm[TCP_CLOSE];
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id);
        desc->comm = temp_comm;
        at_ret_code result = __at_generic_comm_rsp_util(desc, false, true);
        if (result == AT_RSP_TIMEOUT) {
                DEBUG_V0("%s: tcp close command timedout\n", __func__);
                return;
        } else if (result == AT_FAILURE)
                DEBUG_V0("%s: could not close socket,"
                        " connection may be already closed\n", __func__);

        state = IDLE;
        state |= TCP_CONN_CLOSED;
        __at_reset_dl_state();
        return;
}
