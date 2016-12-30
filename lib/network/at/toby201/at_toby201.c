/*
 *  \file at_toby201.c, implementing APIs defined in at.h file
 *
 *  \brief AT command functions for u-blox toby201 lte modem
 *
 *  \copyright Copyright (C) 2016, Verizon. All rights reserved.
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
#include "dbg.h"

typedef enum at_states {
        IDLE = 1,
        WAITING_RESP = 1 << 1,
        NETWORK_LOST = 1 << 2,
        TCP_CONNECTED = 1 << 3,
        TCP_CONN_CLOSED = 1 << 4,
        PROC_RSP = 1 << 5,
        PROC_URC = 1 << 6,
        DL_MODE = 1 << 7,
        TCP_TX = 1 << 8,
        AT_INVALID = 1 << 9
} at_states;

typedef enum at_return_codes {
        AT_SUCCESS = 0,
        AT_RSP_TIMEOUT,
        AT_FAILURE,
        AT_TX_FAILURE,
        AT_TCP_FAILURE,
        AT_WRONG_RSP,
        AT_RECHECK_MODEM
} at_ret_code;

static char *rsp_header = "\r\n";
static char *rsp_trailer = "\r\n";

/* TCP operation can fail at tcp level as well operation level which can be
 * caught with +CME ERROR, while tcp level error requires issuing
 * special command to retrieve error number
 */
static const char *tcp_error = "\r\nERROR\r\n";

static volatile at_states state;
static volatile bool process_rsp;
/* Flag to indicate one time packet data network enable procedure */
static volatile bool pdp_conf;

/* Enable this macro to display messages, error will alway be reported if this
 * macro is enabled while V2 and V1 will depend on debug_level setting
 */
#define DEBUG_AT_LIB

static int debug_level;
/* level v2 is normally for extensive debugging need, for example tracing
 * function calls
 */
#ifdef DEBUG_AT_LIB
#define DEBUG_V2(...)	\
                        do { \
                                if (debug_level >= 2) \
                                        printf(__VA_ARGS__); \
                        } while (0)

/* V1 is normaly used for variables, states which are internal to functions */
#define DEBUG_V1(...) \
                        do { \
                                if (debug_level >= 1) \
                                        printf(__VA_ARGS__); \
                        } while (0)

#define DEBUG_V0(...)	printf(__VA_ARGS__)
#else
#define DEBUG_V0(...)
#define DEBUG_V1(...)
#define DEBUG_V2(...)

#endif

#define DBG_STATE
#ifdef DBG_STATE
#define DEBUG_STATE(...) printf("%s: line %d, state: %u\n",\
                                __func__, __LINE__, (uint32_t)state)
#else
#define DEBUG_STATE(...)
#endif

/* Enable to debug wrong response, this prints expected vs received buffer in
 * raw format
 */
/*#define DEBUG_WRONG_RSP*/

#define CHECK_NULL(x, y) do { \
                                if (!((x))) { \
                                        DEBUG_V1("Fail at line: %d\n", __LINE__); \
                                        return ((y)); \
                                } \
                         } while (0);

#define CHECK_SUCCESS(x, y, z)	\
                        do { \
                                if ((x) != (y)) { \
                                        printf("Fail at line: %d\n", __LINE__); \
                                        return (z); \
                                } \
                        } while (0);

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define IDLE_CHARS	        10
#define MODEM_RESET_DELAY       25000 /* In mili seconds */
/* in mili seconds, polling for modem */
#define CHECK_MODEM_DELAY    1000
/* maximum timeout value in searching for the network coverage */
#define NET_REG_CHECK_DELAY     60000 /* In mili seconds */

/* Waiting arbitrarily as we do not know for how many
 * bytes we will be waiting for when modem does not send wrong response in
 * totality
 */
#define WRONG_RSP_BUF_DUMP_DELAY        2000 /* In mili seconds */

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

static void __at_uart_rx_flush() {
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
                        state = IDLE;
                        state |= TCP_CONN_CLOSED;
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

static void __at_process_urc()
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

static void at_uart_callback(callback_event ev)
{
        switch (ev) {
        case UART_EVENT_RECVD_BYTES:
                if ((state & WAITING_RESP) == WAITING_RESP) {
                        DEBUG_V1("%s: got response\n", __func__);
                        process_rsp = true;
                } else if (((state & PROC_RSP) != PROC_RSP) &&
                        ((state & PROC_URC) != PROC_URC) &&
                        ((state & DL_MODE) != DL_MODE)) {
                                DEBUG_V0("%s: urc from callback:%d\n",
                                        __func__, state);
                                __at_process_urc();
                                /* remote side may disconnect during transmit
                                 * it will send disconnect urc which needs to
                                 * to be processed to detect
                                 *
                                if ((((state & TCP_TX) == TCP_TX) &&
                                        ((state & DL_MODE) == DL_MODE)) ||
                                        (((state & DL_MODE) != DL_MODE))) {

                                }*/
                } else
                        DEBUG_V1("%s:%d:state:%d\n", __func__, __LINE__, state);
                break;
        case UART_EVENT_RX_OVERFLOW:
                DEBUG_V0("%s: rx overflows\n", __func__);
                ASSERT(0);
                __at_uart_rx_flush();
                break;
        default:
                DEBUG_V0("%s: Unknown event code: %d\n", __func__, ev);
                break;
        }

}

static void __at_cleanup(uint8_t flush_uart)
{
        state |= PROC_URC;
        __at_process_urc();
        state &= ~PROC_URC;
        if (flush_uart)
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
        __at_cleanup(1);
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
                                platform_delay(WRONG_RSP_BUF_DUMP_DELAY);
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
        __at_cleanup(1);
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
                platform_delay(WRONG_RSP_BUF_DUMP_DELAY);
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
        __at_cleanup(1);
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
        platform_delay(2000);
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

        DEBUG_V0("%s: resetting modem done...\n", __func__);
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

        bool res = uart_module_init(UART_EN_HW_CTRL, IDLE_CHARS);
        CHECK_SUCCESS(res, true, false)

        uart_set_rx_callback(at_uart_callback);
        state = IDLE;
        process_rsp = false;
        pdp_conf = false;
        __at_uart_rx_flush();
        at_ret_code res_modem;
        /* This may take few seconds */
        res_modem = __at_modem_reset();
        if (res_modem != AT_SUCCESS) {
                DEBUG_V0("%s: modem reset failed\n", __func__);
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

static at_ret_code __at_esc_dl_mode()
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
        int read_bytes;
        int s_id = -1;
        uint8_t i = 0;
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

static at_ret_code __at_pdp_conf()
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
        return s_id;
}

static at_ret_code __at_tcp_tx(const uint8_t *buf, size_t len)
{
        state |= TCP_TX;
        for (int i = 0; i < len; i++) {
                /*FIXME: simulate this scenario for dl mode
                */
                if ((state & TCP_CONNECTED) == TCP_CONNECTED) {
                        if (!__at_uart_write(((uint8_t *)buf + i), 1))
                                return AT_TCP_SEND_FAIL;
                } else
                        break;
        }
        state &= ~TCP_TX;

        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                DEBUG_V0("%s: tcp got deconnected in middle of the write\n",
                        __func__);
                /* URC is already been processed so no need to process it again
                 *
                 */
                /* AT modem returns tcp error 11 which is EAGAIN */
                state &= ~TCP_CONN_CLOSED;
                return AT_TCP_CONNECT_DROPPED;
        }
        return AT_SUCCESS;
}

int at_tcp_send(int s_id, const unsigned char *buf, size_t len)
{
        CHECK_NULL(buf, AT_TCP_INVALID_PARA)
        if ((s_id < 0) || (len == 0))
                return AT_TCP_INVALID_PARA;

        at_ret_code result;
        uint16_t read_bytes;

        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                if ((state & TCP_CONN_CLOSED) == TCP_CONN_CLOSED)
                        return 0;
                DEBUG_V0("%s: tcp not connected\n", __func__);
                return AT_TCP_SEND_FAIL;
        }

        if ((state & DL_MODE) == DL_MODE) {
                result = __at_tcp_tx(buf, len);
                if (result != AT_SUCCESS) {
                        DEBUG_V0("%s:%d: write failed\n", __func__, __LINE__);
                        return AT_TCP_SEND_FAIL;
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
                DEBUG_V0("%s: tcp not connected to read\n", __func__);
                return AT_TCP_RCV_FAIL;
        }

        if ((state & DL_MODE) != DL_MODE) {
                DEBUG_V0("%s:%d: dl mode is off\n", __func__, __LINE__);
                return AT_TCP_RCV_FAIL;
        }
        return uart_rx_available();
}

int at_tcp_recv(int s_id, unsigned char *buf, size_t len)
{
        if (s_id < 0 || !buf) {
                DEBUG_V0("%s: socket or buffer invalid\n", __func__);
                return AT_TCP_INVALID_PARA;
        }
        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                if ((state & TCP_CONN_CLOSED) == TCP_CONN_CLOSED)
                        return 0;
                DEBUG_V0("%s: tcp not connected to recv\n", __func__);
                return AT_TCP_RCV_FAIL;
        }

        if ((state & DL_MODE) == DL_MODE) {
                int line = uart_line_avail(rsp_header, rsp_trailer);
                if (line)
                        DEBUG_V0("%s:line avail in read#####:%d\n", __func__, line);
                int rdb = uart_read(buf, len);
                DEBUG_V1("%s:%d: read:%d, wanted:%d in a dl\n",
                                __func__, __LINE__, rdb, len);
                if (rdb == 0) {
                        DEBUG_V1("%s:%d: read again\n", __func__, __LINE__);
                        errno = EAGAIN;
                        return AT_TCP_RCV_FAIL;
                }
                return rdb;
        } else {
                DEBUG_V0("%s:%d: dl mode not on\n", __func__, __LINE__);
                return AT_TCP_RCV_FAIL;
        }
}

void at_tcp_close(int s_id)
{

        DEBUG_V0("%s:%d\n", __func__, __LINE__);
        if (s_id < 0)
                return;
        __at_dump_buffer(NULL, 0);
        if ((state & DL_MODE) == DL_MODE)
                __at_esc_dl_mode();

        if ((state & TCP_CONN_CLOSED) == TCP_CONN_CLOSED) {
                DEBUG_V0("%s: tcp already closed\n", __func__);
                state = IDLE;
                __at_cleanup(1);
                return;
        }

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
        return;
}
