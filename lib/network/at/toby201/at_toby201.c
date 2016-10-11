/*
 *  \file at_toby201.c, implementing APIs defined in at.h file
 *
 *  \brief AT command functions for u-blox toby201 lte modem
 *
 *  \copyright Copyright (C) 2016, Verizon. All rights reserved.
 *
 */

#include <stm32f4xx_hal.h>
#include "at.h"
#include "at_toby201_command.h"
#include "uart.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef enum at_states {
        IDLE = 1,
        WAITING_RESP = 1 << 1,
        NETWORK_LOST = 1 << 2,
        TCP_CONNECTED = 1 << 3,
        TCP_READ = 1 << 4,
        PROC_RSP = 1 << 5,
        PROC_URC = 1 << 6,
        AT_INVALID = 1 << 7
} at_states;

static char *rsp_header = "\r\n";
static char *rsp_trailer = "\r\n";

static volatile at_states state;
static volatile bool process_rsp;
/* Flag to indicate one time packet data network enable procedure */
static bool pdp_conf;

static int debug_level;
/* level v2 is normally for extensive debugging need, for example tracing
 * function calls
 */
#define DEBUG_V2(...)	if (debug_level >= 2) printf(__VA_ARGS__)
/* V1 is normaly used for variables, states which are internal to functions */
#define DEBUG_V1(...)	if (debug_level >= 1) printf(__VA_ARGS__)
#define DEBUG_V0(...)	printf(__VA_ARGS__)

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

#define CHECK_SUCCESS(x, y, z)	if ((x) != (y)) { \
                                        DEBUG_V1("Fail at line: %d\n", __LINE__); \
                                        return (z); \
                                }
#define CHECK_NULL(x, y)	if (!((x))) { \
                                        DEBUG_V1("Fail at line: %d\n", __LINE__); \
                                        return ((y)); \
                                }

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

#define AT_SUCCESS              0
#define AT_RSP_TIMEOUT          1
#define AT_FAILURE              3
#define AT_WRONG_RSP            4
#define AT_RECHECK_MODEM        5

#define IDLE_CHARS	        10
#define MODEM_RESET_DELAY       60000 /* In mili seconds */
#define NET_REG_CHECK_DELAY     10000 /* In mili seconds */

static uint32_t __at_find_end(char *str, char tail) {
        CHECK_NULL(str, 0)
        char *end = strrchr(str, tail);
        if (!end)
                return 0;
        return end - str;
}

static uint32_t __at_convert_to_decimal(char *num, uint8_t num_digits) {
        CHECK_NULL(num, 0)
        uint16_t multi = 1;
        uint16_t dec = 0;
        for (int i = num_digits - 1 ; i >= 0; i--) {
                dec += ((num[i] - '0') * multi);
                multi *= 10;
        }
        return dec;
}

static int __at_get_bytes(char *rsp_buf, char tail) {
        CHECK_NULL(rsp_buf, -1)
        int num_bytes = -1;
        uint32_t find_end = __at_find_end((char *)rsp_buf, tail);
        if (find_end != 0) {
                num_bytes = __at_convert_to_decimal((char *)rsp_buf, find_end);
                DEBUG_V1("%s: Data Bytes: %d\n", __func__, num_bytes);
        } else
                return -1;
        return num_bytes;
}

static void __at_dump_buffer(uint8_t *buf, uint16_t len)
{
        if (!buf) {
                int bytes = uart_rx_available();
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

static uint8_t __at_process_network_urc(char *urc, at_urc u_code) {

        if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
                return AT_FAILURE;

        uint8_t count = strlen(at_urcs[u_code]);
        uint8_t net_stat = urc[count] - '0';
        DEBUG_V0("%s: net stat: %u\n", __func__, net_stat);
        if (((u_code == NET_STAT_URC) && (net_stat != 1)) ||
                ((u_code == EPS_STAT_URC) && (net_stat == 0)))
                state |= NETWORK_LOST;
        else
                state &= ~NETWORK_LOST;

        /* FIXME: Revisit this if tcp connect still possible after network lost
         * event is detected
         */
        return AT_SUCCESS;
}

static uint8_t __at_process_tcp_close_urc(char *urc) {
        if (strncmp(urc, at_urcs[TCP_CLOSED],
                        strlen(at_urcs[TCP_CLOSED])) == 0) {

                if ((state & TCP_CONNECTED) == TCP_CONNECTED)
                        state &= ~TCP_CONNECTED;
                DEBUG_V1("%s: tcp closed\n", __func__);
                return AT_SUCCESS;
        } else
                return AT_FAILURE;
}

static uint8_t __at_process_tcp_read_urc(char *urc) {

        if (strncmp(urc, at_urcs[DATA_READ], strlen(at_urcs[DATA_READ])) == 0) {
                if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                        DEBUG_V0("%s: tcp not connected\n", __func__);
                        return AT_SUCCESS;
                }
                uint8_t count = strlen(at_urcs[DATA_READ]) + 2;
                if (__at_get_bytes(urc + count, '\r') == -1) {
                        DEBUG_V0("%s: could not get read bytes\n", __func__);
                        return AT_SUCCESS;
                }
                state |= TCP_READ;
                DEBUG_V1("%s: Read bytes available\n", __func__);
        } else
                return AT_FAILURE;

        return AT_SUCCESS;
}

static void __at_process_urc() {

        while (1) {
                uint8_t result = AT_SUCCESS;
                uint16_t read_bytes = uart_line_avail(rsp_header, rsp_trailer);
                if (read_bytes == 0)
                        break;
                uint8_t urc[read_bytes + 1];
                urc[read_bytes] = 0x0;
                if (uart_read(urc, read_bytes) == UART_READ_ERR) {
                        DEBUG_V0("%s: read err (Unlikely)\n", __func__);
                        continue;
                }
                DEBUG_V1("%s: looking to process urc: %s\n",
                                                __func__, (char *)urc);
                result = __at_process_network_urc((char *)urc, NET_STAT_URC);
                if (result == AT_SUCCESS)
                        continue;

                result = __at_process_network_urc((char *)urc, EPS_STAT_URC);
                if (result == AT_SUCCESS)
                        continue;

                result = __at_process_tcp_read_urc((char *)urc);
                if (result != AT_FAILURE)
                        continue;

                result = __at_process_tcp_close_urc((char *)urc);
        }
}

static void at_uart_callback(callback_event ev) {
        switch (ev) {
        case UART_EVENT_RECVD_BYTES:
                if ((state & WAITING_RESP) == WAITING_RESP) {
                        DEBUG_V1("%s: got response\n", __func__);
                        process_rsp = true;
                } else if (((state & PROC_RSP) != PROC_RSP) &&
                        ((state & PROC_URC) != PROC_URC)) {
                        DEBUG_V1("%s: urc processing from callback\n", __func__);
                        __at_process_urc();
                } else
                        DEBUG_STATE();
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

static uint8_t __at_wait_for_rsp(uint32_t *timeout) {

        state |= WAITING_RESP;
        uint8_t result = AT_SUCCESS;
        uint32_t start = HAL_GetTick();
        uint32_t end;
        while (!process_rsp) {
                end = HAL_GetTick();
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

static uint8_t __at_comm_send_and_wait_rsp(char *comm, uint16_t len,
                                                uint32_t *timeout)
{
        /* Process urcs if any before we send down new command
         * and empty buffer
         */
        state |= PROC_URC;
        __at_process_urc();
        state &= ~PROC_URC;
        __at_uart_rx_flush();

        CHECK_NULL(comm, AT_FAILURE)

        if (!__at_uart_write((uint8_t *)comm, len)) {
                DEBUG_V0("%s: uart tx fail\n", __func__);
                return AT_FAILURE;
        }
        return __at_wait_for_rsp(timeout);
}

static uint8_t __at_handle_error_rsp(uint8_t *rsp_buf, uint16_t read_bytes,
                                        at_command_desc *desc, uint8_t rsp_num)
{
        uint8_t i = 0;
        uint8_t result = AT_SUCCESS;

        if (desc->err) {
                if (strncmp((char *)rsp_buf, desc->err,
                                strlen(desc->err)) != 0) {
                        DEBUG_V0("%s: wrong rsp: %s\n",
                                __func__, (char *)rsp_buf);
                        result = AT_WRONG_RSP;
                } else
                        result = AT_FAILURE;
        } else {
                DEBUG_V0("%s: wrong response: %s\n",
                                                __func__, (char *)rsp_buf);
                result = AT_WRONG_RSP;
        }

#ifdef DEBUG_WRONG_RSP
        if (result == AT_WRONG_RSP) {
                DEBUG_V0("%s: Printing received wrong response:\n", __func__);
                __at_dump_buffer(rsp_buf, read_bytes);
                DEBUG_V0("\n");
                DEBUG_V0("%s: Printing expected response:\n", __func__);
                __at_dump_buffer(desc->rsp_desc[rsp_num].rsp,
                                        strlen(desc->rsp_desc[rsp_num].rsp));
                DEBUG_V0("\n");
                if (strncmp(rsp_buf, desc->rsp_desc[rsp_num].rsp,
                        read_bytes) == 0)
                        DEBUG_V0("%s: Received partial response\n", __func__);
        }
#endif
        return result;
}

/* Generic utility to send command, wait for the response, and process response.
 * It is best suited for responses bound by delimiters with the exceptions
 * for write prompt command for tcp write
 */

static uint8_t __at_generic_comm_rsp_util(at_command_desc *desc, bool skip_comm,
                                        bool read_line)
{
        CHECK_NULL(desc, AT_FAILURE)

        uint8_t result = AT_SUCCESS;
        char *comm;
        const char *rsp;
        uint32_t timeout;
        int read_bytes;

        comm = desc->comm;
        timeout = desc->comm_timeout;

        if (!skip_comm) {
                DEBUG_V1("%s: sending %s\n", __func__, comm);
                result = __at_comm_send_and_wait_rsp(comm, strlen(comm),
                                                                &timeout);
                if (result != AT_SUCCESS)
                        goto done;
        }

        state |= PROC_RSP;
        state |= WAITING_RESP;

        for(uint8_t i = 0; i < ARRAY_SIZE(desc->rsp_desc); i++) {
                if (!desc->rsp_desc[i].rsp)
                        continue;
                if (read_line) {
                        read_bytes = uart_line_avail(rsp_header, rsp_trailer);
                        /* Scenario where modem stops sending partial response
                         * before we go out reporting error, give another chance
                         */
                        if (read_bytes == 0) {
                                DEBUG_V1("%s: no line detected yet,"
                                                " wait again for rsp :%d, "
                                                "max remaing timeout: %u\n",
                                                __func__, i, timeout);
                                /* don't care about timeout error here,
                                 * wait for at max remaining time left
                                 */
                                __at_wait_for_rsp(&timeout);
                                read_bytes = uart_line_avail(rsp_header,
                                                                rsp_trailer);
                                if (read_bytes == 0) {
                                        DEBUG_V0("%s: no line available, "
                                                "dumping buffer for debug\n",
                                                __func__);
                                        /* Check if it has partial response */
                                        __at_dump_buffer(NULL, 0);
                                        result = AT_FAILURE;
                                        break;
                                } else
                                        state |= WAITING_RESP;
                        }
                } else
                        read_bytes = uart_rx_available();

                if (read_bytes <= 0) {
                        DEBUG_V0("%s: invalid read_bytes:%d for comm: %s\n",
                                __func__, read_bytes, comm);
                        break;
                }
                uint8_t rsp_buf[read_bytes + 1];
                rsp_buf[read_bytes] = 0x0;

                if (uart_read(rsp_buf, read_bytes) != UART_READ_ERR) {
                        DEBUG_V1("%s: recvd res: %s\n", __func__,
                                                        (char *)rsp_buf);
                        if (strncmp((char *)rsp_buf, desc->rsp_desc[i].rsp,
                                        strlen(desc->rsp_desc[i].rsp)) != 0) {

                                result = __at_handle_error_rsp(rsp_buf,
                                                        read_bytes, desc, i);
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
                        DEBUG_V0("%s: uart read failed (unlikely)\n", __func__);
                        result = AT_FAILURE;
                        break;
                }
        }
        state &= ~PROC_RSP;
        state &= ~WAITING_RESP;
        /* Recommeded to wait atleast 20ms before proceeding */
done:
        HAL_Delay(20);

        /* check to see if we have urcs while command was executing
         * if result was wrong response, chances are that we are out of sync
         */
        DEBUG_V1("%s: Processing URCS outside call back\n", __func__);
        state |= PROC_URC;
        __at_process_urc();
        state &= ~PROC_URC;
        __at_uart_rx_flush();

        DEBUG_V1("%s: result: %d\n", __func__, result);
        return result;
}

static uint8_t __at_modem_reset()
{
        uint8_t result;
        DEBUG_V1("%s: resetting modem...\n", __func__);
        result = __at_generic_comm_rsp_util(&modem_net_status_comm[MODEM_RESET],
                                                                false, true);
        if (result == AT_RSP_TIMEOUT)
                return result;
        HAL_Delay(MODEM_RESET_DELAY);
        DEBUG_V1("%s: resetting modem done...\n", __func__);
        result = __at_generic_comm_rsp_util(
                                &modem_net_status_comm[ECHO_OFF], false, false);
        if (result == AT_RSP_TIMEOUT)
                return result;
        return AT_SUCCESS;
}

static uint8_t __at_check_network_registration()
{
        uint8_t result1 = AT_SUCCESS;
        uint8_t result2 = AT_SUCCESS;
        /* Check network registration status */
        result1 = __at_generic_comm_rsp_util(&modem_net_status_comm[NET_REG_STAT],
                                                false, true);

        /* Check packet switch network registration status */
        result2 = __at_generic_comm_rsp_util(&modem_net_status_comm[EPS_REG_STAT],
                                                false, true);
        if ((result1 == AT_SUCCESS) && (result2 == AT_SUCCESS))
                return AT_SUCCESS;
        return AT_FAILURE;
}

static uint8_t __at_check_modem_conf() {

        uint8_t result = AT_SUCCESS;

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
                if (res == AT_RSP_TIMEOUT)
                        return AT_FAILURE;
                return AT_RECHECK_MODEM;
        }

        result = __at_check_network_registration();
        if (result != AT_SUCCESS) {
                DEBUG_V0("%s: Rechecking network registration\n", __func__);
                HAL_Delay(NET_REG_CHECK_DELAY);
                result = __at_check_network_registration();
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

static uint8_t __at_config_modem() {

        uint8_t result = AT_SUCCESS;
        result =  __at_generic_comm_rsp_util(&modem_net_status_comm[MODEM_OK],
                                                                false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result)

        return __at_check_modem_conf();
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

        /* This may take few seconds */
        uint8_t res_modem = __at_modem_reset();
        CHECK_SUCCESS(res_modem, AT_SUCCESS, false)

        res_modem = __at_config_modem();
        if (res_modem == AT_RECHECK_MODEM) {
                DEBUG_V1("%s: Rechecking modem config after reset\n", __func__);
                res_modem = __at_config_modem();
        }

        if (res_modem != AT_SUCCESS) {
                state = AT_INVALID;
                return false;
        }
        return true;

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
        uint8_t result = AT_SUCCESS;
        at_command_desc *desc = &tcp_comm[TCP_CONF];
        desc->rsp_desc[0].data = &s_id;

        /* Configure tcp connection first to be tcp client */
        result = __at_generic_comm_rsp_util(desc, false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, -1);
        s_id = *((int *)desc->rsp_desc[0].data);
        if (s_id < 0) {
                DEBUG_V0("%s: could not get socket: %d\n", __func__, s_id);
                return -1;
        }

        /* Now make remote connection */
        desc = &tcp_comm[TCP_CONN];
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id,
                host, port);

        desc->comm = temp_comm;
        result = __at_generic_comm_rsp_util(desc, false, true);

        CHECK_SUCCESS(result, AT_SUCCESS, -1);

        state |= TCP_CONNECTED;
        DEBUG_V1("%s: socket:%d created\n", __func__, s_id);
        return s_id;
}

static uint8_t __at_pdp_conf()
{
        uint8_t result = __at_generic_comm_rsp_util(
                                        &pdp_conf_comm[SEL_IPV4_PREF],
                                        false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result)
        return __at_generic_comm_rsp_util(&pdp_conf_comm[ACT_PDP], false, true);
}

int at_tcp_connect(const char *host, const char *port)
{

        CHECK_NULL(host, -1)
        if (state > IDLE) {
                DEBUG_V0("%s: TCP connect not possible, state :%u\n",
                        __func__, state);
                return -1;
        }
        if (!pdp_conf) {
                if (__at_pdp_conf() != AT_SUCCESS) {
                        DEBUG_V0("%s: PDP configuration failed\n", __func__);
                        return -1;
                }
                pdp_conf = true;

        }
        /* call will update state to TCP_CONNECTED if successful */
        return __at_tcp_connect(host, port);
}

static void __at_parse_tcp_sock_stat(void *rcv_rsp, int rcv_rsp_len,
                                        const char *stored_rsp, void *data)
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
        /* 5 is to count in 0,10,4 to get to 4 in the command response
         * 4 is status code to determine if tcp connection is still active
         * This situation may arise from queue full
         */
        count += 5;
        *((int *)data) = (*((char *)(rcv_rsp + count))) - '0';
}

static void __at_parse_tcp_send_rsp(void *rcv_rsp, int rcv_rsp_len,
                                        const char *stored_rsp, void *data)
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
        /* 2 is to get to number of write bytes in the tcp write command
         * response for example: 0,10\r\n where 0 is socket
         */
        count += 2;
        int num_bytes = __at_get_bytes(rcv_rsp + count, '\r');
        if (num_bytes == -1) {
                DEBUG_V0("%s: sent bytes invalid\n", __func__);
                *((int *)data) = -1;
                return;
        }
        *((int *)data) = num_bytes;
}

static int __at_handle_tcp_send_error(int s_id)
{

        int data = -1;
        at_command_desc *desc = &tcp_comm[TCP_SOCK_STAT];
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id);
        desc->comm = temp_comm;
        desc->rsp_desc[0].data = &data;
        uint8_t result = __at_generic_comm_rsp_util(desc, false, true);
        if (result != AT_SUCCESS) {
                DEBUG_V0("%s: command failed, result code:%d\n",
                        __func__, result);
                return AT_TCP_SEND_FAIL;
        }
        data = *((int *)desc->rsp_desc[0].data);
        if (data != TCP_SOCK_STATUS_CODE) {
                DEBUG_V0("%s: connection closed, code: %d\n", __func__, data);
                state &= ~TCP_CONNECTED;
                return AT_TCP_CONNECT_DROPPED;
        }
        return AT_TCP_SEND_FAIL;

}

int at_tcp_send(int s_id, const unsigned char *buf, size_t len)
{
        CHECK_NULL(buf, AT_TCP_SEND_FAIL)
        if (s_id < 0)
                return AT_TCP_SEND_FAIL;

        uint16_t result;
        uint16_t read_bytes;

        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                DEBUG_V0("%s: tcp not connected\n", __func__);
                return AT_TCP_CONNECT_DROPPED;
        }

        at_command_desc *desc = &tcp_comm[TCP_WRITE_PROMPT];

        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id, len);
        desc->comm = temp_comm;
        DEBUG_V1("%s: sending write prompt\n", __func__);
        result = __at_generic_comm_rsp_util(desc, false, false);
        if (result != AT_SUCCESS) {
                DEBUG_V0("%s: command failure\n", __func__);
                /* Queue may be full */
                if (result == AT_FAILURE)
                        return __at_handle_tcp_send_error(s_id);
        }

        /* Recommeneded in datasheet */
        HAL_Delay(50);

        for (int i = 0; i < len; i++) {
                if ((state & TCP_CONNECTED) == TCP_CONNECTED)
                        __at_uart_write(((uint8_t *)buf + i), 1);
                else
                        break;
        }

        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                DEBUG_V0("%s: tcp got deconnected in middle of the write\n",
                        __func__);
                /* URC is already been processed so no need to process it again
                 *
                 */
                return AT_TCP_CONNECT_DROPPED;
        }
        desc = &tcp_comm[TCP_SEND];
        uint32_t timeout = desc->comm_timeout;

        result = __at_wait_for_rsp(&timeout);
        CHECK_SUCCESS(result, AT_SUCCESS, AT_TCP_SEND_FAIL)

        /* Parse the response for session or socket id and number of write */
        int data = -1;
        desc->rsp_desc[0].data = &data;
        DEBUG_V1("%s: processing write rsp after complete write\n", __func__);
        result = __at_generic_comm_rsp_util(desc, true, true);
        CHECK_SUCCESS(result, AT_SUCCESS, AT_TCP_SEND_FAIL)
        return data;
}

static void __at_parse_read_qry_rsp(void *rcv_rsp, int rcv_rsp_len,
                                        const char *stored_rsp, void *data)
{
        if (!data)
                return;
        int num_read = -1;
        uint16_t start = strlen(stored_rsp);
        if (start > rcv_rsp_len) {
                DEBUG_V0("%s: stored rsp is bigger then received rsp\n",
                        __func__);
                *((int *)data) = -1;
                return;
        }
        /* 2 is to account for socket id and "," in the command response
         * For example: +USORD: 0,10\r\n where 0 is socket id
         */
        start += 2;
        num_read = __at_get_bytes(rcv_rsp + start, '\r');
        if (num_read == -1) {
                DEBUG_V0("%s: can not find number of read bytes\n", __func__);
                *((int *)data) = -1;
                return;
        }
        *((int *)data) = num_read;
}

int at_read_available(int s_id) {

        int data = -1;
        uint8_t result = AT_SUCCESS;
        at_command_desc *desc = &tcp_comm[TCP_RCV_QRY];

        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
               DEBUG_V0("%s: tcp not connected to read\n", __func__);
               return AT_TCP_CONNECT_DROPPED;
        }
        if (s_id < 0) {
               DEBUG_V0("%s: Invalid socket\n", __func__);
               return AT_TCP_RCV_FAIL;
        }
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id);
        desc->comm = temp_comm;
        desc->rsp_desc[0].data = &data;
        result = __at_generic_comm_rsp_util(desc, false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, AT_TCP_RCV_FAIL)
        DEBUG_V1("%s: total read bytes: %d\n", __func__, data);
        return data;
}

static void __at_parse_rcv_err(uint8_t *rsp_buf, uint16_t r_idx,
                        at_command_desc *desc)
{
        if (uart_read(rsp_buf + r_idx, strlen(desc->err) - r_idx) !=
                UART_READ_ERR) {
                if (strncmp(rsp_buf, desc->err, strlen(desc->err)) != 0)
                        DEBUG_V0("%s:wrong rsp: %s\n",
                                        __func__, (char *)rsp_buf);
                else
                        DEBUG_V0("%s: got error\n", __func__);
        }
}

static int __at_parse_rcv_rsp(uint8_t *buf)
{
        int num_read = -1;
        char temp_buf[TEMP_COMM_LIMIT];
        memset(temp_buf, 0, TEMP_COMM_LIMIT);
        int r_idx = 0;
        bool found = false;
        while (r_idx < TEMP_COMM_LIMIT && !found) {
                if (uart_read(temp_buf + r_idx, 1) != UART_READ_ERR) {
                        if (temp_buf[r_idx] == '"')
                                found = true;
                        else
                                r_idx++;
                } else
                        break;

        }
        if (!found) {
                DEBUG_V0("%s: read marker not found\n", __func__);
                return AT_TCP_RCV_FAIL;
        }

        num_read = __at_get_bytes(temp_buf + 2, ',');
        if (num_read == -1) {
                DEBUG_V0("%s: number of read not found\n", __func__);
                return AT_TCP_RCV_FAIL;
        }

        DEBUG_V1("%s: Reading bytes: %d\n", __func__, num_read);
        if (uart_read(buf, num_read) == UART_READ_ERR) {
                DEBUG_V0("%s:%d uart read error\n", __func__, __LINE__);
                return AT_TCP_RCV_FAIL;
        }

        const char *ok = "\r\nOK\r\n";
        memset(temp_buf, 0, TEMP_COMM_LIMIT);
        if (uart_read(temp_buf, strlen(ok) + 3) == UART_READ_ERR) {
                DEBUG_V0("%s:%d uart read error\n", __func__, __LINE__);
                return AT_TCP_RCV_FAIL;
        }
        /* 3 is to account for /r/n" at the end of the data read and before
         * start of the ok
         */
        if (strncmp(temp_buf + 3, ok, strlen(ok)) != 0) {
                DEBUG_V0("%s: non ok after reading into buffer\n", __func__);
                return AT_TCP_RCV_FAIL;
        }
        return num_read;
}

int at_tcp_recv(int s_id, unsigned char *buf, size_t len)
{
        if (s_id < 0 || !buf) {
                DEBUG_V0("%s: socket or buffer invalid\n", __func__);
                return AT_TCP_RCV_FAIL;
        }
        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                DEBUG_V0("%s: tcp not connected to recv\n", __func__);
                return AT_TCP_CONNECT_DROPPED;
        }
        if ((state & TCP_READ) != TCP_READ) {
                DEBUG_V0("%s: read not possible at this time\n", __func__);
                return AT_TCP_RCV_FAIL;
        }

        uint8_t result = AT_SUCCESS;
        int r_bytes = AT_TCP_RCV_FAIL;
        uint16_t read_bytes;
        uint32_t timeout;

        at_command_desc *desc = &tcp_comm[TCP_RCV];

        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id, len);
        DEBUG_V1("%s: sending command: %s\n", __func__, temp_comm);
        timeout = desc->comm_timeout;
        result = __at_comm_send_and_wait_rsp(temp_comm, strlen(temp_comm),
                                                &timeout);
        CHECK_SUCCESS(result, AT_SUCCESS, AT_TCP_RCV_FAIL);

        read_bytes = uart_rx_available();
        if (read_bytes <= 0) {
                DEBUG_V0("%s: comm: %s failed\n", __func__, temp_comm);
                return AT_TCP_RCV_FAIL;
        }
        uint8_t rsp_buf[read_bytes + 1];
        rsp_buf[read_bytes] = 0x0;
         /* header plus first byte, + means response and E means error */
        uint16_t r_idx =  strlen(rsp_header) + 1;
        if (uart_read(rsp_buf, r_idx) != UART_READ_ERR) {
                if (strncmp((char *)(rsp_buf), desc->rsp_desc[0].rsp, r_idx) != 0) {
                        if (strncmp((char *)rsp_buf, desc->err, r_idx) != 0)
                                DEBUG_V0("%s:%d wrong rsp\n",
                                                        __func__, __LINE__);
                        else
                                __at_parse_rcv_err(rsp_buf, r_idx, desc);
                        goto done;
                } else {
                        if (uart_read(rsp_buf + r_idx,
                                strlen(desc->rsp_desc[0].rsp) - r_idx) !=
                                        UART_READ_ERR) {
                                if (strncmp(rsp_buf, desc->rsp_desc[0].rsp,
                                        strlen(desc->rsp_desc[0].rsp)) != 0) {
                                        DEBUG_V0("%s:%d, wrong rsp: %s\n",
                                                __func__, __LINE__,
                                                (char *)rsp_buf);
                                        goto done;
                                }
                                r_bytes = __at_parse_rcv_rsp(buf);
                                goto done;
                        }
                }

        } else
                DEBUG_V0("%s: uart read error\n", __func__);

done:
        HAL_Delay(20);
        state |= PROC_URC;
        __at_process_urc();
        state &= ~PROC_URC;
        __at_uart_rx_flush();
        return r_bytes;
}

void at_tcp_close(int s_id) {

        at_command_desc *desc = &tcp_comm[TCP_CLOSE];
        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                DEBUG_V0("%s: tcp already closed\n", __func__);
                return;
        }
        if (s_id < 0)
                return;

        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id);
        desc->comm = temp_comm;
        uint8_t result = __at_generic_comm_rsp_util(desc, false, true);
        if (result == AT_RSP_TIMEOUT) {
                DEBUG_V0("%s: tcp close command timeout\n", __func__);
                return;
        } else if (result == AT_FAILURE)
                DEBUG_V0("%s: could not close socket,"
                        " connection may be already closed\n", __func__);

        state &= ~TCP_CONNECTED;
        return;
}
