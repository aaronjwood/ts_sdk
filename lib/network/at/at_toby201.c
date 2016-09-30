/*
 *  \file at_toby201.c
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

#define AT_SUCCESS      0
#define AT_RSP_TIMEOUT  1
#define AT_FAILURE      3
#define AT_WRONG_RSP    4

typedef enum at_states {
        IDLE = 1,
        WAITING_RESP = 1 << 1,
        NETWORK_LOST = 1 << 2,
        TCP_CONNECTED = 1 << 3,
        TCP_WRITE = 1 << 4,
        TCP_READ = 1 << 5,
        PROC_RSP = 1 << 6,
        PROC_URC = 1 << 7,
        AT_INVALID = 1 << 8
} at_states;

static char *rsp_header = "\r\n";
static char *rsp_trailer = "\r\n";

static volatile at_states state;
static volatile bool process_rsp;
static bool pdp_conf;
static int num_read_bytes;

static int debug_level;
#define DEBUG_V2(...)	if (debug_level >= 2) printf(__VA_ARGS__)
#define DEBUG_V1(...)	if (debug_level >= 1) printf(__VA_ARGS__)
#define DEBUG_V0(...)	printf(__VA_ARGS__)

#ifdef DBG_STATE
#define DEBUG_STATE(...) printf("%s: line %d, state: %u\n",\
                                __func__, __LINE__, (uint32_t)state)
#else
#define DEBUG_STATE(...)
#endif

#define CHECK_SUCCESS(x, y, z)	if ((x) != (y)) { \
                                        DEBUG_V1("Fail at line: %d\n", __LINE__); \
                                        return (z); \
                                }

static uint8_t __at_parse_tcp_conf_rsp(uint8_t *rsp_buf, int read_bytes,
                                        at_command_desc *desc , uint8_t rsp_num);
static uint8_t __at_parse_tcp_send_rsp(uint8_t *rsp_buf, int read_bytes,
                                        at_command_desc *desc , uint8_t rsp_num);
static uint8_t __at_parse_read_qry_rsp(uint8_t *rsp_buf, int read_bytes,
                                        at_command_desc *desc , uint8_t rsp_num);

static uint32_t __at_find_end(char *str, char tail) {
        char *end = strrchr(str, tail);
        if (!end)
                return 0;
        return end - str;
}

static uint32_t __at_convert_to_decimal(char *num, uint8_t num_digits) {
        uint16_t multi = 1;
        uint16_t dec = 0;
        for (int i = num_digits - 1 ; i >= 0; i--) {
                dec += ((num[i] - '0') * multi);
                multi *= 10;
        }
        return dec;
}

static int __at_get_bytes(char *rsp_buf, uint16_t start, char tail) {

        int num_bytes = -1;
        uint32_t find_end = __at_find_end((char *)rsp_buf + start, tail);
        if (find_end != 0) {
                num_bytes = __at_convert_to_decimal((char *)rsp_buf + start,
                                                find_end);
                DEBUG_V1("%s: Data Bytes: %d\n", __func__, num_bytes);
        } else
                return -1;
        return num_bytes;
}

static void __at_uart_rx_flush() {
        uart_flush_rx_buffer();
}

static bool __at_uart_write(uint8_t *buf, uint16_t len) {
        return uart_tx(buf, len, AT_TX_WAIT_MS);
}

static uint8_t __at_process_network_urc(char *urc, at_urc u_code) {
        if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0) {
                DEBUG_V2("%s: no matched urc\n", __func__);
                return AT_FAILURE;
        }

        uint8_t count = strlen(at_urcs[u_code]);
        uint8_t net_stat = urc[count] - '0';
        printf("%s: net stat: %u\n", __func__, net_stat);
        if ((u_code == NET_STAT_URC && net_stat != 1) ||
                (u_code == EPS_STAT_URC && net_stat == 0))
                state |= NETWORK_LOST;
        else
                state &= ~NETWORK_LOST;

        return AT_SUCCESS;
}

static uint8_t __at_process_tcp_close_urc(char *urc) {
        if (strncmp(urc, at_urcs[TCP_CLOSED],
                strlen(at_urcs[TCP_CLOSED])) == 0) {
                if (state & TCP_CONNECTED == TCP_CONNECTED)
                        state &= ~TCP_CONNECTED;
                else
                        DEBUG_V0("%s: tcp cant be closed\n", __func__);
                return AT_SUCCESS;
        } else {
                DEBUG_V0("%s: no matched urc\n", __func__);
                return AT_FAILURE;
        }
}

static uint8_t __at_process_tcp_read_urc(char *urc) {

        if (strncmp(urc, at_urcs[DATA_READ], strlen(at_urcs[DATA_READ])) == 0) {
                if (state & TCP_CONNECTED != TCP_CONNECTED) {
                        DEBUG_V0("%s: tcp not connected\n", __func__);
                        num_read_bytes = 0;
                        return AT_SUCCESS;
                }
                uint8_t count = strlen(at_urcs[DATA_READ]) + 2;
                num_read_bytes = __at_get_bytes(urc, count, '\r');
                if (num_read_bytes == -1) {
                        DEBUG_V0("%s: data read failure\n", __func__);
                        num_read_bytes = 0;
                        return AT_SUCCESS;
                }
                state |= TCP_READ;
                DEBUG_V0("%s: Read bytes left: %d\n", __func__, num_read_bytes);
        } else
                return AT_FAILURE;

        return AT_SUCCESS;
}

static uint8_t __at_process_urc() {

        while (1) {
                uint8_t result = AT_SUCCESS;
                uint16_t read_bytes = uart_line_avail(rsp_header, rsp_trailer);
                if (read_bytes == 0)
                        break;
                uint8_t urc[read_bytes];
                if (uart_read(urc, read_bytes) == UART_READ_ERR) {
                        DEBUG_V0("%s: read err\n", __func__);
                        return AT_FAILURE;
                }
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
        return AT_SUCCESS;
}

static void at_uart_callback(callback_event ev) {
        switch (ev) {
        case UART_EVENT_RECVD_BYTES:
                if (state & WAITING_RESP == WAITING_RESP)
                        process_rsp = true;
                else if ((state & PROC_RSP != PROC_RSP) &&
                        (state & PROC_URC != PROC_URC))
                        __at_process_urc();
                else
                        DEBUG_STATE();
                break;
        case UART_EVENT_RX_OVERFLOW:
                DEBUG_V0("%s: rx overflows\n", __func__);
                break;
        default:
                DEBUG_V0("%s: AUC\n", __func__);
                break;
        }

}

static uint8_t __at_wait_for_rsp(uint32_t timeout) {

        uint8_t result = AT_SUCCESS;
        state |= WAITING_RESP;
        uint32_t start = HAL_GetTick();
        while (!process_rsp) {
                if ((HAL_GetTick() - start) > timeout) {
                        result = AT_RSP_TIMEOUT;
                        DEBUG_V0("%s: RSP_TIMEOUT\n", __func__);
                        break;
                }
        }
        state &= ~(WAITING_RESP);
        process_rsp = false;
        return result;
}

static uint8_t __at_comm_send_and_wait_rsp(char *comm, uint16_t len,
        uint32_t timeout) {

        /* Process urcs if any before we send down new command
         * and empty buffer
         */
        state |= PROC_URC;
        __at_process_urc();
        state &= ~PROC_URC;
        __at_uart_rx_flush();

        if (!__at_uart_write(comm, len)) {
                DEBUG_V0("%s: uart tx fail\n", __func__);
                return AT_FAILURE;
        }
        return __at_wait_for_rsp(timeout);
}

static uint8_t __at_parse_rsp(uint8_t *rsp_buf, int read_bytes,
                                at_command_desc *desc , uint8_t rsp_num) {

        switch (desc->rsp_desc[rsp_num].parse_rsp) {
        case TCP_CONF:
                return __at_parse_tcp_conf_rsp(rsp_buf, read_bytes,
                        desc, rsp_num);
        case TCP_SEND:
                return __at_parse_tcp_send_rsp(rsp_buf, read_bytes,
                        desc, rsp_num);
        case TCP_RCV_QRY:
                return __at_parse_read_qry_rsp(rsp_buf, read_bytes,
                        desc, rsp_num);
        default:
                return AT_SUCCESS;
        }
}

static uint8_t __at_generic_comm_rsp_util(at_command_desc *desc, bool skip_comm,
                                        bool read_line) {

        if (!desc)
                return AT_FAILURE;

        uint8_t result = AT_SUCCESS;
        char *comm;
        const char *rsp;
        uint32_t timeout;
        int read_bytes;

        comm = desc->comm;
        timeout = desc->comm_timeout;

        if (!skip_comm) {
                result = __at_comm_send_and_wait_rsp(comm, strlen(comm),
                                                                timeout);
                CHECK_SUCCESS(result, AT_SUCCESS, result)
        }
        state |= PROC_RSP;
        DEBUG_STATE();
        uint8_t i = 0;
        for(; i < (sizeof(desc->rsp_desc)/sizeof(*(desc->rsp_desc))); i++) {
                if (!desc->rsp_desc[i].rsp)
                        continue;
                if (read_line)
                        read_bytes = uart_line_avail(rsp_header, rsp_trailer);
                else
                        read_bytes = uart_rx_available();

                if (read_bytes <= 0) {
                        DEBUG_V0("%s: comm: %s failed (Unlikely)\n",
                                                                __func__, comm);
                        state &= ~PROC_RSP;
                        DEBUG_STATE();
                        return AT_FAILURE;
                }
                uint8_t rsp_buf[read_bytes + 1];
                rsp_buf[read_bytes] = 0x0;
                if (uart_read(rsp_buf, read_bytes) != UART_READ_ERR) {
                        if (strncmp((char *)rsp_buf, desc->rsp_desc[i].rsp,
                                        strlen(desc->rsp_desc[i].rsp)) != 0) {

                                if (strncmp((char *)rsp_buf, desc->error,
                                                strlen(desc->error)) != 0) {
                                        DEBUG_V0("%s: wrong response: %s\n",
                                                __func__, (char *)rsp_buf);
                                        result = AT_WRONG_RSP;
                                } else
                                        result = AT_FAILURE;
                                break;
                        } else
                                result = __at_parse_rsp(rsp_buf, read_bytes,
                                                                desc, i);

                } else
                        result = AT_FAILURE;
        }
        state &= ~PROC_RSP;

        /* check to see if we have urcs while command was executing
         * if result was wrong response, chances are that we are out of sync
         */

        if ((result != AT_WRONG_RSP) &&
                uart_line_avail(rsp_header, rsp_trailer) > 0) {
                state |= PROC_URC;
                __at_process_urc();
                state &= ~PROC_URC;
        }
        __at_uart_rx_flush();
        DEBUG_STATE();
        return result;
}

static uint8_t __at_check_modem_conf() {

        if (state & IDLE != IDLE) {
                DEBUG_V2("%s: state not valid: %u\n", __func__, (uint32_t)state);
                return AT_FAILURE;
        }

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
         * for it and reset the modem
         */
        result = __at_generic_comm_rsp_util(&modem_net_status_comm[MNO_STAT],
                                                false, true);
        if (result == AT_WRONG_RSP) {
                // Dipen: FIXME
                /* Configure modem for verizon and reset it here, poll for modem
                 * to comeback by sending AT command
                 */
        }

        /* Check network registration status */
        result = __at_generic_comm_rsp_util(&modem_net_status_comm[NET_REG_STAT],
                                                false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result)

        /* Check packet switch network registration status */
        result = __at_generic_comm_rsp_util(&modem_net_status_comm[EPS_REG_STAT],
                                                false, true);

        CHECK_SUCCESS(result, AT_SUCCESS, result)
        return result;

}

static uint8_t __at_check_modem() {
        if (state & IDLE != IDLE) {
                DEBUG_V2("%s: state not valid: %u\n", __func__, (uint32_t)state);
                return AT_FAILURE;
        }
        return __at_generic_comm_rsp_util(&modem_net_status_comm[MODEM_OK],
                                        false, true);
}

bool at_init() {

        bool res = uart_module_init(true, 5);
        CHECK_SUCCESS(res, true, false)

        uart_set_rx_callback(at_uart_callback);
        state = AT_INVALID;
        process_rsp = false;
        pdp_conf = false;
        __at_uart_rx_flush();

        uint8_t res_modem = __at_check_modem();
        CHECK_SUCCESS(res_modem, AT_SUCCESS, false)

        res_modem = __at_check_modem_conf();
        CHECK_SUCCESS(res_modem, AT_SUCCESS, false)
        state = IDLE;
        return true;

}

static uint8_t __at_parse_tcp_conf_rsp(uint8_t *rsp_buf, int read_bytes,
                                at_command_desc *desc , uint8_t rsp_num) {

        if (rsp_num > MAX_RSP_LINE || !desc)
                return AT_FAILURE;

        uint16_t count = strlen(desc->rsp_desc[rsp_num].rsp);
        if (count >= read_bytes) {
                DEBUG_V0("%s: stored rsp is bigger then received rsp\n",
                        __func__);
                return AT_FAILURE;
        }

        int s_id = (*(char *)(rsp_buf + count)) - '0';
        desc->rsp_desc[rsp_num].data = s_id;
        return AT_SUCCESS;
}

static int __at_tcp_connect(const char *host, int port) {
        int s_id = -1;
        int read_bytes;
        uint8_t i = 0;
        uint8_t result = AT_SUCCESS;
        at_command_desc *desc = &tcp_comm[TCP_CONF];

        /* Configure tcp connection first to be tcp client */
        result = __at_generic_comm_rsp_util(desc, false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, -1);

        s_id = desc->rsp_desc[0].data;
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

static uint8_t __at_pdp_conf() {
        uint8_t result = __at_generic_comm_rsp_util(
                                &pdp_conf_comm[SEL_IPV4_PREF], false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result)
        //Dipen: check back about second response line
        result = __at_generic_comm_rsp_util(&pdp_conf_comm[ACT_PDP],
                                                false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, result)
}

int at_tcp_connect(const char *host, int port) {
        state |= PROC_URC;
        __at_process_urc();
        state &= ~PROC_URC;

        if (state > IDLE) {
                DEBUG_V0("%s: TCP connect not possible, state :%u\n",
                        __func__, state);
                return -1;
        }
        if (!pdp_conf) {
                if (__at_pdp_conf() == AT_SUCCESS)
                        pdp_conf = true;
                else {
                        DEBUG_V0("%s: PDP not configured\n", __func__);
                        return -1;
                }
        }
        /* call will update state to TCP_CONNECTED if successful */
        return __at_tcp_connect(host, port);
}

static uint8_t __at_parse_tcp_send_rsp(uint8_t *rsp_buf, int read_bytes,
                                at_command_desc *desc , uint8_t rsp_num) {


        if (rsp_num > MAX_RSP_LINE || !desc)
                return AT_FAILURE;

        uint16_t count = strlen(desc->rsp_desc[rsp_num].rsp);
        if (count >= read_bytes) {
                DEBUG_V0("%s: stored rsp is bigger then received rsp\n",
                        __func__);
                return AT_FAILURE;
        }
        count += 2;
        int num_bytes = __at_get_bytes(rsp_buf, count, '\r');
        if (num_bytes == -1) {
                DEBUG_V0("%s: sent bytes invalid\n", __func__);
                desc->rsp_desc[rsp_num].data = num_bytes;
                return AT_FAILURE;
        }
        desc->rsp_desc[rsp_num].data = num_bytes;
        return AT_SUCCESS;
}

int at_tcp_send(int s_id, const unsigned char *buf, size_t len) {
        uint16_t result;
        uint16_t read_bytes;

        /* In binary write mode, it has to wait for below response
         * before proceding
         */
        const char *write_prompt = "\r\n@";

        if (len < MAX_TCP_SEND_BYTES) {
                DEBUG_V0("%s: cant send more then: %d\n", MAX_TCP_SEND_BYTES);
                return -1;
        }
        if (state & TCP_CONNECTED != TCP_CONNECTED) {
                DEBUG_V0("%s: tcp not connected\n", __func__);
                return -1;
        }

        at_command_desc *desc = &tcp_comm[TCP_SEND];

        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id, len);

        result = __at_comm_send_and_wait_rsp(temp_comm, strlen(temp_comm), 20);
        CHECK_SUCCESS(result, AT_SUCCESS, -1)

        read_bytes = uart_rx_available();
        if (read_bytes == 0) {
                DEBUG_V0("%s: comm: %s failed\n", __func__, temp_comm);
                return -1;
        }
        uint8_t rsp_buf[read_bytes + 1];
        rsp_buf[read_bytes] = 0x0;
        if (uart_read(rsp_buf, read_bytes) == UART_READ_ERR) {
                DEBUG_V0("%s: comm: %s failed\n", __func__, temp_comm);
                return -1;
        }

        if (strncmp(rsp_buf, write_prompt, strlen(write_prompt)) != 0) {
                DEBUG_V0("%s: wrong response: %s failed\n", __func__, rsp_buf);
                return -1;
        }
        /* recommeneded in datasheet */
        HAL_Delay(50);

        state |= TCP_WRITE;
        for (int i = 0; i < len; i++) {
                if ((state & TCP_CONNECTED) == TCP_CONNECTED)
                        __at_uart_write(((uint8_t *)buf + i), 1);
                else
                        break;
        }

        if ((state & TCP_CONNECTED) != TCP_CONNECTED) {
                //Dipen:FIXME simulate this scenario in test cases which is to terminate connection
                // in a middle of write
                state &= ~TCP_WRITE;
                return -1;
        }

        result = __at_wait_for_rsp(desc->comm_timeout);
        state &= ~TCP_WRITE;
        CHECK_SUCCESS(result, AT_SUCCESS, -1)

        /* Parse the response for session or socket id and number of write */
        result = __at_generic_comm_rsp_util(desc, true, true);
        CHECK_SUCCESS(result, AT_SUCCESS, -1)
        return desc->rsp_desc[0].data;
}

static uint8_t __at_parse_read_qry_rsp(uint8_t *rsp_buf, int read_bytes,
        at_command_desc *desc , uint8_t rsp_num) {

        if (rsp_num > MAX_RSP_LINE || !desc)
                return AT_FAILURE;

        int num_read = -1;
        uint16_t start = strlen(desc->rsp_desc[rsp_num].rsp);
        if (start >= read_bytes) {
                DEBUG_V0("%s: stored rsp is bigger then received rsp\n",
                        __func__);
                return AT_FAILURE;
        }
        start += 2;
        num_read = __at_get_bytes(rsp_buf, start, '\r');
        if (num_read == -1) {
                DEBUG_V0("%s: can not find number of read bytes\n", __func__);
                return AT_FAILURE;
        }
        desc->rsp_desc[rsp_num].data = num_read;
        return AT_SUCCESS;
}

int at_read_available(int s_id) {
        uint8_t result = AT_SUCCESS;
        at_command_desc *desc = &tcp_comm[TCP_RCV_QRY];

        if (state & TCP_CONNECTED != TCP_CONNECTED) {
                DEBUG_V0("%s: tcp not connected to read\n", __func__);
                return -1;
        }
        if (s_id < 0) {
                DEBUG_V0("%s: Invalid socket id\n", __func__);
                return -1;
        }
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id);
        desc->comm = temp_comm;
        result = __at_generic_comm_rsp_util(desc, false, true);
        CHECK_SUCCESS(result, AT_SUCCESS, -1)

        return tcp_comm[TCP_RCV_QRY].rsp_desc[0].data;
}

static int __at_parse_rcv_rsp(int s_id, uint8_t *rsp_buf, uint16_t start,
                                uint8_t *buf, size_t len) {
        int num_read = -1;
        int s_temp = (*(rsp_buf + start)) - '0';
        if (s_temp != s_id) {
                DEBUG_V0("%s: wrong s_id :%d\n", __func__, s_temp);
                /* Dipen: drain rx buffer? */
                return -1;
        }
        start += 2;
        num_read = __at_get_bytes(rsp_buf, start, ',');
        if (num_read == -1) {
                DEBUG_V0("%s: can not find number of read bytes\n", __func__);
                return -1;
        }
        uint32_t find_end = __at_find_end((char *)rsp_buf + start, ',');
        start = start + find_end + 2;
        memcpy(buf, rsp_buf + start, num_read);
        start += 3;
        const char *ok = "\r\nOK\r\n";
        if (strncmp(rsp_buf, ok, strlen(ok)) != 0) {
                DEBUG_V0("%s: non ok\n", __func__);
        }
        return num_read;
}

int at_tcp_recv(int s_id, unsigned char *buf, size_t len) {

        uint8_t result = AT_SUCCESS;
        int r_bytes = 0;
        uint16_t read_bytes;

        at_command_desc *desc = &tcp_comm[TCP_RCV];
        if (state & TCP_CONNECTED != TCP_CONNECTED) {
                DEBUG_V0("%s: tcp not connected: %u\n", __func__);
                //Dipen: wot to return?
                return -1;
        }

        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id, len);

        result = __at_comm_send_and_wait_rsp(temp_comm, strlen(temp_comm),
                                                desc->comm_timeout);
        CHECK_SUCCESS(result, AT_SUCCESS, -1);

        read_bytes = uart_rx_available();
        if (read_bytes <= 0) {
                DEBUG_V0("%s: comm: %s failed\n", __func__, temp_comm);
                return -1;
        }
        uint8_t rsp_buf[read_bytes];
        if (uart_read(rsp_buf, read_bytes) != UART_READ_ERR) {
                if (strncmp((char *)rsp_buf, desc->rsp_desc[0].rsp,
                                        strlen(desc->rsp_desc[0].rsp)) != 0) {
                        if (strncmp((char *)rsp_buf, desc->error,
                                strlen(desc->error)) != 0)
                                result = AT_WRONG_RSP;
                        else
                                result = AT_FAILURE;
                } else {
                        uint16_t start = strlen(desc->rsp_desc[0].rsp);
                        r_bytes = __at_parse_rcv_rsp(s_id, rsp_buf, start,
                                                        buf, len);
                }

        } else
                result = AT_FAILURE;
        if (result == AT_WRONG_RSP) {
                DEBUG_V0("%s: wrong response\n", __func__);
                /* we may be out of sync with underlyig buffer */
                __at_uart_rx_flush();
        }
        CHECK_SUCCESS(result, AT_SUCCESS, -1);
        return r_bytes;
}

void at_tcp_close(int s_id) {

        at_command_desc *desc = &tcp_comm[TCP_CLOSE];
        if (state & TCP_CONNECTED != TCP_CONNECTED) {
                DEBUG_V1("%s: tcp already closed\n", __func__);
                return;
        }
        if (s_id < 0)
                return;

        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm_sketch, s_id);
        desc->comm = temp_comm;

        if (__at_generic_comm_rsp_util(desc, false, true) != AT_SUCCESS) {
                DEBUG_V0("%s: could not close connection\n", __func__);
                return;
        }
        state &= ~TCP_CONNECTED;
        return;
}
