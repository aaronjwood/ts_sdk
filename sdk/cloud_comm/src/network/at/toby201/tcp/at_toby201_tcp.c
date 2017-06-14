/*
 *  \file at_toby201_tcp.c, implementing APIs defined in at_tcp.h file
 *
 *  \brief API implementation that uses the UBLOX TOBY-L201 modem to communicate over TCP
 *
 *  \copyright Copyright (C) 2016, 2017 Verizon. All rights reserved.
 *
 */

#include <errno.h>
#include "at_tcp.h"
#include "at_toby201_tcp_command.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "sys.h"
#include "dbg.h"

/*
 * Intermediate buffer to hold data from uart buffer when disconnect string
 * is detected fully, disconnect string is from the dl mode
 */
typedef struct _at_intr_buf {
        /* This buffer does not overflow as it is only being filled when whole
         * dl disconnect string sequence is detected, total capacity of this
         * buffer can not exceed beyond UART_RX_BUFFER_SIZE - disconnect string
         */
        uint8_t buf[UART_BUF_SIZE];
        buf_sz ridx;
        buf_sz buf_unread;
} at_intr_buf;

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

/* Flag to indicate one time packet data network enable procedure */
static volatile bool pdp_conf;

/* maximum timeout value in searching for the network coverage */
#define NET_REG_CHECK_DELAY     60000 /* In milli seconds */

/*
 * FIXME: After setting the PDP context, it seems the modem needs some time to
 * stabilize after responding with an OK. Without this wait, the command to
 * create a socket (usually the very next command) fails with a generic error
 * (ERROR) even with AT+CMEE = 1. This behavior needs to be investigated further.
 */
#define PDP_CTX_STABLE_MS	500

static at_ret_code __at_process_dl_close_urc(const char *urc, at_urc u_code)
{
        if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) == 0) {
                state &= ~DL_MODE;
                DEBUG_V0("%s:%d: direct link mode closed\n", __func__, __LINE__);
                return AT_SUCCESS;
        } else
                return AT_FAILURE;
}

static at_ret_code __at_process_pdp_tcp_close_urc(const char *urc, at_urc u_code)
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
			default:
				break;
		}
	}
	return AT_FAILURE;
}

static void __at_lookup_dl_esc_str(void)
{
        int nw_f;
        int cur_f = 0;
        uint8_t itr = 0;
        for (; itr < strlen(dl.dis_str); itr++) {
                nw_f = at_core_find_pattern(-1, (uint8_t *)dl.dis_str, itr + 1);
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
	buf_sz sz = at_core_rx_available();
	if (sz == 0) {
		DEBUG_V0("%d:UERR\n", __LINE__);
		dl.dl_buf.buf_unread = 0;
		return;
	}
	DEBUG_V0("%d:SZ_A:%d\n", __LINE__, sz);
	/* 3 is to account for trailing x\r\n, where x is socket id */
	if ((sz - (strlen(dl.dis_str) + 3)) == 0)
		at_core_clear_rx();
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
				if (at_core_read((uint8_t *)dl.dl_buf.buf +
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
		if (at_core_read(temp_buf, 3) != 3) {
			DEBUG_V0("%d:UERR\n", __LINE__);
			goto done;
		}
		/* Since we processed all the disconnect string, there may be
		 * other URCs buffered in the modem while we were in DL Mode
		 */
		at_core_process_urc(true);

		return;
done:
		dl.dl_buf.buf_unread = 0;
		dl.dl_buf.ridx = 0;
		at_core_clear_rx();
		return;
        }
}

static void at_uart_callback(void)
{
	if (((state & DL_MODE) == DL_MODE) &&
			((state & TCP_DL_RX) != TCP_DL_RX)) {
		if (dl.dis_state < DIS_FULL) {
			__at_lookup_dl_esc_str();
			if (dl.dis_state == DIS_FULL) {
				state = TCP_REMOTE_DISCONN;
				__at_xfer_to_buf();
			}
		}
	} else if (!at_core_is_proc_rsp() && !at_core_is_proc_urc() &&
			((state & DL_MODE) != DL_MODE)) {
		DEBUG_V1("%s: urc from callback:%d\n",
				__func__, state);
		at_core_process_urc(false);
	} else {
		DEBUG_V1("%s:%d:state:%d\n", __func__, __LINE__, state);
	}
}

static void urc_callback(const char *urc)
{
	at_ret_code result = __at_process_pdp_tcp_close_urc(urc, TCP_CLOSED);
	if (result == AT_SUCCESS)
		return;

	result = __at_process_dl_close_urc(urc, DISCONNECT);
	if (result == AT_SUCCESS)
		return;

	result = __at_process_pdp_tcp_close_urc(urc, PDP_DEACT);
}

static inline at_ret_code __at_check_network_registration()
{
	if (at_core_query_netw_reg())
                return AT_SUCCESS;
        return AT_FAILURE;
}

static at_ret_code __at_modem_conf()
{
        at_ret_code result = AT_SUCCESS;

        /* set escape sequence delay */
        char temp_comm[TEMP_COMM_LIMIT];
        at_command_desc *desc = &tcp_comm[ESCAPE_TIME_CONF];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, DL_MODE_ESC_TIME);
        desc->comm = temp_comm;
        result = at_core_wcmd(desc, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result);

        /* Check MNO configuration, if it is not set for the Verizon, configure
         * for it and reset the modem to save settings
         */
        result = at_core_wcmd(&modem_net_status_comm[MNO_STAT], true);
        if (result != AT_SUCCESS) {
                result = at_core_wcmd(&modem_net_status_comm[MNO_SET], true);
                CHECK_SUCCESS(result, AT_SUCCESS, result);
                uint8_t res = at_core_modem_reset();
                if (res == AT_RSP_TIMEOUT || res == AT_TX_FAILURE)
                        return res;
                return AT_RECHECK_MODEM;
        }

        result = AT_FAILURE;
        uint32_t start = sys_get_tick_ms();
        uint32_t end;
        while (result != AT_SUCCESS) {
                end = sys_get_tick_ms();
                DEBUG_V0("%s: Rechecking network registration\n", __func__);
                if ((end - start) > NET_REG_CHECK_DELAY) {
                        DEBUG_V0("%s: timed out\n", __func__);
                        break;
                }
                result = __at_check_network_registration();
                sys_delay(CHECK_MODEM_DELAY);
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

	bool res = at_core_init(at_uart_callback, urc_callback);
	CHECK_SUCCESS(res, true, false);

        state = IDLE;
        pdp_conf = false;
        /* This may take few seconds */
        at_ret_code res_modem = at_core_modem_reset();
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
        sys_delay(DL_ESC_TIME_MS);
        at_command_desc *desc = &tcp_comm[ESCAPE_DL_MODE];
        at_ret_code result = at_core_wcmd(desc, true);
        if (AT_COMM_DELAY_MS < DL_ESC_TIME_MS)
                sys_delay(DL_ESC_TIME_MS - DL_ESC_TIME_MS);

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
        at_ret_code result = at_core_wcmd(desc, true);
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
        return at_core_wcmd(desc, true);
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
        result = at_core_wcmd(desc, true);
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
        result = at_core_wcmd(desc, true);

        CHECK_SUCCESS(result, AT_SUCCESS, AT_CONNECT_FAILED);

        state |= TCP_CONNECTED;
        state &= ~TCP_CONN_CLOSED;
        DEBUG_V0("%s: socket:%d created\n", __func__, s_id);
        return s_id;
}

static at_ret_code __at_pdp_conf(void)
{
	at_ret_code result = AT_FAILURE;
#if SIM_TYPE == M2M
	result = at_core_wcmd(&pdp_conf_comm[ADD_PDP_CTX], true);
	CHECK_SUCCESS(result, AT_SUCCESS, result);
	result = at_core_wcmd(&pdp_conf_comm[ACT_PDP_CTX], true);
	CHECK_SUCCESS(result, AT_SUCCESS, result);
	result = at_core_wcmd(&pdp_conf_comm[MAP_PDP_PROFILE], true);
	CHECK_SUCCESS(result, AT_SUCCESS, result);
#endif
	result = at_core_wcmd(&pdp_conf_comm[SEL_IPV4], true);
	CHECK_SUCCESS(result, AT_SUCCESS, result);
	return at_core_wcmd(&pdp_conf_comm[ACT_PDP_PROFILE], true);
}

int at_tcp_connect(const char *host, const char *port)
{

        CHECK_NULL(host, -1);
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
		sys_delay(PDP_CTX_STABLE_MS);
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
                        if (!at_core_write(((uint8_t *)buf + i), 1))
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
        CHECK_NULL(buf, AT_TCP_INVALID_PARA);
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
        return at_core_rx_available();
}

static int __at_calc_new_len(void)
{
        buf_sz n_sz = at_core_rx_available();
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
	switch (dl.dis_state) {
	case DIS_PARTIAL:
                DEBUG_V1("%s:%d: unchanged state\n",__func__, __LINE__);
                return DL_PARTIAL_SUC;
	case DIS_FULL:
                DEBUG_V1("%s:%d: partial became full\n", __func__, __LINE__);
                return DL_PARTIAL_TO_FULL;
	case DIS_IDLE:
                DEBUG_V1("%s:%d: fake partial detected\n",__func__, __LINE__);
		/* Fall through */
	default:
                return DL_PARTIAL_ERROR;
	}
}

static int __at_process_rcv_dl_partial(size_t len)
{
        buf_sz prev_matched = dl.matched_bytes;
        buf_sz prev_pos = dl.l_matched_pos;
        buf_sz sz = at_core_rx_available();

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

                sys_delay(DL_PARTIAL_WAIT);

                r = __at_test_dl_partial_state();
                if (r != DL_PARTIAL_SUC)
                        return r;

                /* chances that last bytes are binary remnants , check if it
                 * still partial state assuming during that delay new bytes have
                 * arrived
                 */
                if (prev_pos != dl.l_matched_pos) {
                        int res = __at_calc_new_len();
                        if (res == DL_PARTIAL_ERROR)
                                return DL_PARTIAL_ERROR;
                        DEBUG_V0("%s:%d: new available: (%u)\n",
                                __func__, __LINE__, res);
                        a_avail = (buf_sz)res;
                } else {
                        if (prev_matched == dl.matched_bytes) {
                                /* no change in data, consider this as a
                                 * binary blob and reset dl escape mode
                                 * state machine
                                 */
                                a_avail = at_core_rx_available();
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
                                len = at_core_rx_available();
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
                int rdb = at_core_read(buf, len);
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
		at_core_process_urc(true);
		at_core_clear_rx();
		state = IDLE;
		return;
	}

	if ((state & DL_MODE) == DL_MODE)
		__at_esc_dl_mode();


	at_command_desc *desc = &tcp_comm[TCP_CLOSE];
	char temp_comm[TEMP_COMM_LIMIT];
	snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id);
	desc->comm = temp_comm;
	at_ret_code result = at_core_wcmd(desc, true);
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
