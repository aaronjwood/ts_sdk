/*
 *  AT command functions for u-blox toby201 lte modem
 *
 *  Copyright (C) 2016, Verizon. All rights reserved.
 *
 */
#include "at.h"
#include "at_toby201.h"
#include "uart.h"
#include <stdint.h>
#include <stdbool.h>

#define AT_SUCCESS      0
#define AT_RSP_TIMEOUT  1
#define AT_WRONG_RSP    2
#define AT_FAILURE      3
#define AT_READ_FAILURE 4

#define INVOKE_CALLBACK(x)	if (at_cb) at_cb((x))
#define CHECK_SUCCESS(x, y, z)	if ((x) != (y)) return (z)

typedef enum at_states {
        IDLE = 0x0,
        WAITING_RESP = 1 << 1,
        PROC_RESP = 1 << 2,
        DATA_MODE = 1 << 3,
        NETWORK_LOST = 1 << 4,
        TCP_CONNECTED = 1 << 5,
        TCP_WRITE = 1 << 6,
        TCP_READ = 1 << 7
} at_states;

static const char *at_urcs[] = {
                [NET_STAT_URC] = "\r\n+CEREG: ",
                [EPS_STAT_URC] = "\r\n+UREG: ",
                [NO_CARRIER] = "\r\nNO CARRIER\r\n",
                [DATA_READ] = "\r\n+UUSORD: ",
                [TCP_CLOSED] = "\r\n+UUSOCL: "
};

static at_command_desc modem_net_status_comm[MOD_END] = {
        [MODEM_OK] = {"at\r", "\r\nOK\r\n", NULL, 20},
        [NET_STAT] = {"at+cereg=1\r", "\r\nOK\r\n", NULL, 20},
        [EPS_STAT] = {"at+ureg=1\r", "\r\nOK\r\n", NULL, 20},
        [MNO_STAT] = {"at+umnoconf?\r", "\r\n+UMNOCONF: 3,23\r\n\r\nOK\r\n",
                        NULL, 100},
        [MNO_SET] = {"at+umnoconf=3,23\r", "\r\n+UMNOCONF: 3,23\r\n\r\nOK\r\n",
                        NULL, 100},
        [SIM_READY] = {"at+cpin?\r", "\r\n+CPIN: READY\r\n\r\nOK\r\n",
                        "\r\nERROR\r\n", 100},
        [NET_REG_STAT] = {"at+cereg?\r", "\r\n+CEREG: 1,1\r\n\r\nOK\r\n",
                        NULL, 20},
        [EPS_REG_STAT] = {"at+ureg?\r", "\r\n+UREG: 1,7\r\n\r\nOK\r\n",
                        NULL, 20}
};

static at_command_desc pdp_conf_comm[PDP_END] = {
        [SEL_IPV4_PREF] = {"at+upsd=0,0,2\r", "\r\nOK\r\n", NULL, -1},
        [ACT_PDP] = {"at+upsda=0,3\r", "\r\nOK\r\n\r\n+UUPSDA: 0,", "ERROR", 150}
};

static at_command_desc tcp_comm[TCP_END] = {
        [TCP_CONF] = {"at+usocr=6\r", "\r\n+USOCR: ", "\r\nERROR\r\n", -1}, //Dipen: FIXME for error hanlding
        [TCP_CONN] = {"at+usoco=%d,%s,%d\r", "\r\nOK\r\n", "\r\nERROR\r\n", -1, 0},
        [TCP_SEND] = {"at+usowr=%d,%d\r", "+USOWR: 0,%d\r\n\r\nOK",
                        "ERROR", 10},
        [TCP_RCV] = {"at+usord=%d,%d\r", "+USORD: %d,%d,",
                        "ERROR", 10}
};

#define TEMP_COMM_LIMIT 16
#define MAX_RSP_BYTES   64      /* bytes to store single line of response */
static uint8_t rsp_line[64];

static const char *rsp_header = "\r\n";
static const char *rsp_trailer = "\r\n";

static volatile at_states state;
static volatile bool process_rsp;
static volatile bool read_available;
static bool pdp_conf;

static uint16_t num_read_bytes;

static void at_uart_callback(callback_event ev) {
        switch (ev) {
        case UART_EVENT_RESP_RECVD:
                if (state & WAITING_RESP == WAITING_RESP)
                        process_rsp = true;
                else {
                        /* need to process all the URCs here */
                        __at_process_urc();
                }
                break;
        default:
                printf("AUC\n");
                break;
        }

}

static void __at_uart_rx_flush() {
        uart_flush_rx_buffer();
}

static bool __at_uart_write(const char *comm) {
        uart_write(comm, strlen(comm));
}

static uint8_t __at_process_network_urc(char *urc, at_urc u_code) {
        if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
                return AT_FAILURE;

        uint8_t count = strlen(at_urcs[u_code]);
        uint8_t net_stat = urc[count] - '0';
        printf("net stat: %u\n", net_stat);
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
                        printf("tcp cant be closed when not connected\n");
                return AT_SUCCESS;
        } else
                return AT_FAILURE;
}

static uint8_t __at_process_tcp_read_urc(char *urc) {
        uint8_t result = AT_SUCCESS;
        if (strncmp(urc, at_urcs[DATA_READ], strlen(at_urcs[DATA_READ])) == 0) {
                if (state & TCP_CONNECTED != TCP_CONNECTED) {
                        printf("Cant read when tcp is not connected\n");
                        num_read_bytes = 0;
                        return AT_READ_FAILURE;
                }
                //Dipen: FIXME, test this
                uint8_t count = strlen(at_urcs[DATA_READ]) + 2;
                uint8_t find_end = count;
                bool cr_lf = false;
                while (1) {
                        if (urc[find_end] == '\r') {
                                if (urc[find_end + 1] != '\n') {
                                        cr_lf = false;
                                } else
                                        cr_lf = true;
                                break;
                        } else
                                find_end++;
                }
                if (cr_lf) {
                        uint8_t read_num[find_end - count];
                        memcpy(read_num, ((uint8_t *)urc + count,
                                find_end - count);
                        uint16_t multi = 1;
                        uint16_t temp_read_bytes = 0;
                        for (int i = sizeof(read_num) - 1 ; i >= 0; i--) {
                                temp_read_bytes += (read_num[i] * multi);
                                multi *= 10;
                        }
                        printf("read bytes left: %d\n", temp_read_bytes);
                        num_read_bytes = temp_read_bytes;
                        state |= TCP_READ;
                } else {
                        printf("data read failure\n");
                        result = AT_READ_FAILURE;
                }
        } else
                result = AT_FAILURE;

        return result;
}

static uint8_t __at_process_urc() {
        uint8_t result = AT_SUCCESS;
        while (1) {
                read_bytes = uart_rx_line_available(rsp_header, rsp_trailer);
                if (read_bytes == 0)
                        break;
                uint8_t urc[read_bytes];
                if (uart_read(urc_line, read_bytes) == UART_READ_ERR) {
                        printf("APU_READ_ERR\n");
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
                if (result == AT_SUCCESS)
                        continue;
        }
        return result;
}

static uint8_t __at_wait_for_rsp(const char *rsp, uint32_t timeout) {

        uint8_t result = AT_SUCCESS;
        state |= WAITING_RESP;
        int start = millis();
        while (!process_rsp && timeout != 0) {
                if ((millis() - start) > timeout) {
                        result = AT_RSP_TIMEOUT;
                        break;
                }
        }
        state &= ~(WAITING_RESP);
        if (result == AT_SUCCESS) {
                process_rsp = false;
                state |= PROC_RESP;
                uint16_t read_bytes = uart_rx_line_available(rsp_header,
                        rsp_trailer);
                if (read_bytes > 0 && read_bytes < MAX_RSP_BYTES) {
                        rsp_line[read_bytes] = 0x0;
                        if (uart_read(rsp_line, read_bytes) != UART_READ_ERR) {
                                if (strncmp((char *)rsp_line, rsp,
                                strlen(rsp)) != 0)
                                        result = AT_WRONG_RSP;
                        } else {
                                result = AT_FAILURE;
                                printf("AWR_1\n");
                        }
                } else {
                        result = AT_FAILURE;
                        printf("AWR_2\n");
                }
        }
        state &= ~(PROC_RESP);
        return result;
}

static uint8_t __at_comm_send_and_wait_rsp(at_command_desc *desc) {
        uint8_t result;
        uint16_t read_bytes;
        state |= PROC_URC;
        result = __at_process_urc();
        state &= ~PROC_URC;
        __at_uart_write(desc->comm);
        return __at_wait_for_rsp(desc->rsp, desc->rsp_timeout);
}

static uint8_t __at_check_modem_conf() {

        if (state != IDLE)
                return AT_FAILURE;

        uint8_t result = AT_SUCCESS;

        /* Enable EPS network status URCs */
        result = __at_comm_send_and_wait_rsp(
                &modem_net_status_comm[EPS_STAT]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);

        /* ENABLE network status URCs */
        result = __at_comm_send_and_wait_rsp(
                &modem_net_status_comm[NET_STAT]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);


        /* Check if simcard is inserted */
        result = __at_comm_send_and_wait_rsp(&modem_net_status_comm[SIM_READY]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);

        /* Check MNO configuration, if it is not set for the Verizon, configure
         * for it and reset the modem
         */
        result = __at_comm_send_and_wait_rsp(&modem_net_status_comm[MNO_STAT]);
        if (result == AT_WRONG_RSP) {
                // Dipen: FIXME
                /* Configure modem for verizon and reset it here, poll for modem
                 * to comeback by sending AT command
                 */
        }

        /* Check network registration status */
        result = __at_comm_send_and_wait_rsp(
                &modem_net_status_comm[NET_REG_STAT]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);

        /* Check packet switch network registration status */
        result = __at_comm_send_and_wait_rsp(
                &modem_net_status_comm[EPS_REG_STAT]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);

        return result;

}

static uint8_t __at_pdp_conf() {
        uint8_t result = __at_comm_send_and_wait_rsp(
                &pdp_conf_comm[SEL_IPV4_PREF]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);
        result = __at_comm_send_and_wait_rsp(&pdp_conf_comm[ACT_PDP]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);
}

static int __at_tcp_connect(const char *host, int port) {
        int s_id = -1;
        uint8_t result = __at_comm_send_and_wait_rsp(
                &tcp_comm[TCP_CONF]);
        CHECK_SUCCESS(result, AT_SUCCESS, -1);
        uint16_t count = strlen(modem_net_status_comm[TCP_CONF].rsp);
        s_id = (*(char *)(rsp_line + count)) - '0';
        uint16_t read_bytes = uart_line_available();
        if (read_bytes == 0) {
                return -1;
        }
        uint8_t temp_rsp_line[read_bytes];
        temp_rsp_line[read_bytes] = 0x0;
        if (uart_rx_read(temp_rsp_line, read_bytes) == UART_READ_ERR) {
                printf("ATC_READ_ERR\n");
                return -1;
        }
        if (strncmp(temp_rsp_line, "\r\nOK\r\n", 6) != 0) {
                printf("ATC_Rsp_err: %s\n", (char *)temp_rsp_line);
                return -1;
        }
        //Dipen: FIXME, test this, what does it send actually
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, tcp_comm[TCP_CONN].comm,
                s_id, host, port);
        tcp_comm[TCP_CONN].comm = temp_comm;
        result = __at_comm_send_and_wait_rsp(&tcp_comm[TCP_CONN]);
        CHECK_SUCCESS(result, AT_SUCCESS, -1);
        state |= TCP_CONNECTED;
        return s_id;

}

static uint8_t __at_check_modem() {

        return __at_comm_send_and_wait_rsp(&modem_net_status_comm[MODEM_OK]);
}

bool at_init() {

        bool res = uart_module_init(true,5);
        CHECK_SUCCESS(res, true, false);

        uart_set_rx_callback(at_uart_callback);
        state = IDLE;
        process_rsp = false;
        process_urc = false;
        pdp_conf = false;
        __at_uart_rx_flush();

        uint8_t res_modem = __at_check_modem();
        CHECK_SUCCESS(res_modem, AT_SUCCESS, false);

        res_modem = __at_check_modem_conf();
        CHECK_SUCCESS(res_modem, AT_SUCCESS, false);

        return true;

}

int at_tcp_send(int s_id, const unsigned char *buf, size_t len) {
        __at_process_urc();
        if (state & TCP_CONNECTED != TCP_CONNECTED) {
                printf("ATS not connected\n", );
                return -1;
        }
        //Dipen: FIXME, test this, what does it send actually
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, modem_net_status_comm[TCP_CONN].comm,
                s_id, host, port);
        modem_net_status_comm[TCP_CONN].comm = temp_comm;
        tcp_comm[TCP_SEND].comm
        __at_uart_write(desc->comm);
        return __at_wait_for_rsp(desc->rsp, desc->rsp_timeout);
        __at
}

int at_tcp_connect(const char *host, int port) {
        switch (state) {
                case IDLE:
                case TCP_READ:
                        if (!pdp_conf) {
                                if (__at_pdp_conf() == AT_SUCCESS)
                                        pdp_conf = true;
                                else
                                        return -1;
                        }
                        return __at_tcp_connect(host, port);
                case TCP_CONNECTED:
                case TCP_WRITE:
                case DATA_MODE:
                default:
                        printf("TCP connect not possible\n");
                        return -1;
        }
}
