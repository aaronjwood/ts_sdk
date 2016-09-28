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
#include <string.h>

#define AT_SUCCESS      0
#define AT_RSP_TIMEOUT  1
#define AT_FAILURE      3
#define AT_WRONG_RSP    4

#define INVOKE_CALLBACK(x)	if (at_cb) at_cb((x))
#define CHECK_SUCCESS(x, y, z)	if ((x) != (y)) return (z)

typedef enum at_states {
        IDLE = 0x0,
        WAITING_RESP = 1 << 1,
        NETWORK_LOST = 1 << 2,
        TCP_CONNECTED = 1 << 3,
        TCP_WRITE = 1 << 4,
        TCP_READ = 1 << 5
} at_states;

static const char *at_urcs[URC_END] = {
                [NET_STAT_URC] = "\r\n+CEREG: ",
                [EPS_STAT_URC] = "\r\n+UREG: ",
                [NO_CARRIER] = "\r\nNO CARRIER\r\n",
                [DATA_READ] = "\r\n+UUSORD: ",
                [TCP_CLOSED] = "\r\n+UUSOCL: "
};

static at_command_desc modem_net_status_comm[MOD_END] = {
        [MODEM_OK] = {
                .comm = "at\r",
                .rsp = {"\r\nOK\r\n", NULL}
                .error =  NULL,
                .rsp_timeout = 20
        },
        [NET_STAT] = {
                .comm = "at+cereg=1\r",
                .rsp = {"\r\nOK\r\n", NULL},
                .error = NULL,
                .rsp_timeout = 20
        },
        [EPS_STAT] = {
                .comm = "at+ureg=1\r",
                .rsp = {"\r\nOK\r\n", NULL},
                .error = NULL,
                .rsp_timeout = 20
        },
        [MNO_STAT] = {
                .comm = "at+umnoconf?\r",
                .rsp = {"\r\n+UMNOCONF: 3,23","\r\n\r\nOK\r\n"},
                .error = NULL,
                .rsp_timeout = 100
        },
        [MNO_SET] = {
                .comm = "at+umnoconf=3,23\r",
                .rsp =  {"\r\n+UMNOCONF: 3,23\r\n","\r\nOK\r\n"},
                .error = NULL,
                .rsp_timeout = 100
        },
        [SIM_READY] = {
                .comm = "at+cpin?\r",
                .rsp = {"\r\n+CPIN: READY\r\n","\r\nOK\r\n"},
                .error = "\r\nERROR\r\n",
                .rsp_timeout = 100
        },
        [NET_REG_STAT] = {
                .comm = "at+cereg?\r",
                .rsp = {"\r\n+CEREG: 1,1\r\n","\r\nOK\r\n"},
                .error = NULL,
                .rsp_timeout = 20
        },
        [EPS_REG_STAT] = {
                .comm = "at+ureg?\r",
                .rsp = {"\r\n+UREG: 1,7\r\n","\r\nOK\r\n"},
                .error = NULL,
                .rsp_timeout = 20
        }
};

static at_command_desc pdp_conf_comm[PDP_END] = {
        [SEL_IPV4_PREF] = {
                .comm = "at+upsd=0,0,2\r",
                .rsp = {"\r\nOK\r\n", NULL},
                .error = NULL,
                .rsp_timeout = 20
        },
        [ACT_PDP] = {
                .comm = "at+upsda=0,3\r",
                .rsp = {"\r\nOK\r\n","\r\n+UUPSDA: 0,"},
                .error = "\r\nERROR\r\n",
                .rsp_timeout = 150000
        }
};

static at_command_desc tcp_comm[TCP_END] = {
        [TCP_CONF] = {
                .comm = "at+usocr=6\r",
                .rsp = {"\r\n+USOCR: ", "\r\nOK\r\n"},
                .error = "\r\nERROR\r\n",
                .rsp_timeout = 20
        }, //Dipen: FIXME for error hanlding
        [TCP_CONN] = {
                .comm = "at+usoco=%d,%s,%d\r",
                .rsp = {"\r\nOK\r\n", NULL},
                .error = "\r\nERROR\r\n",
                .rsp_timeout = 20000
        },
        [TCP_SEND] = {
                .comm = "at+usowr=%d,%d\r",
                .rsp = {"\r\n+USOWR: ","\r\nOK\r\n"},
                .error = "\r\nERROR\r\n",
                .rsp_timeout = 10000
        },
        [TCP_RCV] = {
                .comm = "at+usord=%d,%d\r",
                .rsp = {"\r\n+USORD: ", "\r\nOK\r\n"},
                .error = "\r\nERROR\r\n",
                .rsp_timeout = 10000
        },
        [TCP_CLOSE] = {
                .comm = "at+usocl=%d\r",
                .rsp = {"\r\nOK\r\n", NULL},
                .error = "\r\nERROR\r\n",
                .rsp_timeout = 10000
        }
};

static const char *rsp_header = "\r\n";
static const char *rsp_trailer = "\r\n";

static volatile at_states state;
static volatile bool process_rsp;
static bool pdp_conf;

static uint8_t read_buffer[MAX_TCP_RCV_BYTES];
static uint16_t read_ptr;
static uint16_t write_ptr;

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

static bool __at_uart_write(const char *buf, uint16_t len, at_type type) {
        return uart_tx((uint8_t *)buf), len, AT_TX_WAIT_MS);
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

static uint8_t __at_process_tcp_read_urc(char *urc) {
        uint8_t result = AT_SUCCESS;
        if (strncmp(urc, at_urcs[DATA_READ], strlen(at_urcs[DATA_READ])) == 0) {
                if (state & TCP_CONNECTED != TCP_CONNECTED) {
                        printf("Cant read when tcp is not connected\n");
                        num_read_bytes = 0;
                        return AT_READ_FAILURE;
                }
                uint8_t count = strlen(at_urcs[DATA_READ]) + 2;
                uint16_t find_end = __at_find_end(urc + count, '\r');

                if (find_end != 0) {
                        uint16_t temp_read_bytes =
                                __at_convert_to_decimal(urc + count, find_end);
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
        }
        return result;
}

static uint8_t __at_wait_for_rsp(uint32_t timeout) {
        uint8_t result = AT_SUCCESS;
        state |= WAITING_RESP;
        uint32_t start = HAL_GetTick();
        while (!process_rsp && timeout != 0) {
                if ((HAL_GetTick() - start) > timeout) {
                        result = AT_RSP_TIMEOUT;
                        break;
                }
        }
        state &= ~(WAITING_RESP);
        return result;
}

static uint8_t __at_comm_send_and_wait_rsp(const char *comm, uint16_t len,
        uint32_t timeout) {

        __at_process_urc();
        if (!__at_uart_write(comm, len) {
                printf("ACSAWR_TX_FAIL\n");
                return AT_FAILURE;
        }
        return __at_wait_for_rsp(timeout);
}

static __at_generic_rsp_checker(at_command_desc *desc) {

        uint8_t result = AT_SUCCESS;
        const char *comm;
        const char *rsp;
        uint32_t timeout;
        int read_bytes;
        uint16_t begin = 0;

        comm = desc->comm;
        rsp = desc->rsp;
        timout = desc->rsp_timeout;

        result = __at_comm_send_and_wait_rsp(comm, strlen(comm), timeout);
        CHECK_SUCCESS(result, AT_SUCCESS, result);
        uint8_t i = 0;
        for(; i < (sizeof(desc->rsp)/sizeof(*(desc->rsp))); i++) {
                if (!desc->rsp[i])
                        continue;
                read_bytes = uart_line_avail(rsp_header, rsp_trailer);
                if (read_bytes <= 0) {
                        printf("%s: comm: %s failed\n", __func__, comm);
                        result = AT_FAILURE;
                        continue;
                }
                uint8_t rsp_buf[read_bytes + 1];
                rsp_buf[read_bytes] = 0x0;
                if (uart_read(rsp_buf, read_bytes) != UART_READ_ERR) {
                        if (strncmp((char *)rsp_buf, desc->rsp[i],
                        strlen(desc->rsp[i])) != 0) {
                                printf("%s: wrong response: %s\n", __func__,
                                (char *)rsp_buf);
                                result = AT_WRONG_RSP;
                                continue;
                        }

                } else
                        result = AT_FAILURE;
        }
        return result;
}

static uint8_t __at_check_modem_conf() {

        if (state & IDLE != IDLE)
                return AT_FAILURE;

        uint8_t result = AT_SUCCESS;

        /* Enable EPS network status URCs */
        result = __at_generic_rsp_checker(&modem_net_status_comm[EPS_STAT]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);

        /* ENABLE network status URCs */
        result = __at_generic_rsp_checker(&modem_net_status_comm[NET_STAT]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);

        /* Check if simcard is inserted */
        result = __at_generic_rsp_checker(&modem_net_status_comm[SIM_READY]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);

        /* Check MNO configuration, if it is not set for the Verizon, configure
         * for it and reset the modem
         */
        result = __at_generic_rsp_checker(&modem_net_status_comm[MNO_STAT]);
        if (result == AT_WRONG_RSP) {
                // Dipen: FIXME
                /* Configure modem for verizon and reset it here, poll for modem
                 * to comeback by sending AT command
                 */
        }

        /* Check network registration status */
        result = __at_generic_rsp_checker(&modem_net_status_comm[NET_REG_STAT]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);

        /* Check packet switch network registration status */
        result = __at_generic_rsp_checker(&modem_net_status_comm[EPS_REG_STAT]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);

        return result;

}

static uint8_t __at_pdp_conf() {
        uint8_t result = __at_generic_rsp_checker(
                                &pdp_conf_comm[SEL_IPV4_PREF]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);
        //Dipen: check back about second response line
        result = __at_generic_rsp_checker(&pdp_conf_comm[ACT_PDP]);
        CHECK_SUCCESS(result, AT_SUCCESS, result);
}

static uint8_t __at_check_modem() {

        return __at_generic_rsp_checker(&modem_net_status_comm[MODEM_OK]);
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

static int __at_tcp_connect(const char *host, int port) {
        int s_id = -1;
        int read_bytes;
        uint8_t i = 0;
        uint8_t result = AT_SUCCESS;
        at_command_desc *desc = &tcp_comm[TCP_CONF];

        /* Configure tcp connection first to be tcp client */
        result = __at_comm_send_and_wait_rsp(desc->comm, strlen(desc->comm),
                                                        desc->rsp_timeout);
        CHECK_SUCCESS(result, AT_SUCCESS, -1);

        /* Parse the response for session or socket id */
        for(; i < (sizeof(desc->rsp)/sizeof(*(desc->rsp))); i++) {
                if (!desc->rsp[i])
                        continue;
                read_bytes = uart_line_avail(rsp_header, rsp_trailer);
                if (read_bytes <= 0) {
                        printf("%s: comm: %s failed\n", __func__, comm);
                        result = AT_FAILURE;
                        continue;
                }
                uint8_t rsp_buf[read_bytes + 1];
                rsp_buf[read_bytes] = 0x0;
                if (uart_read(rsp_buf, read_bytes) != UART_READ_ERR) {
                        if (strncmp((char *)rsp_buf, desc->rsp[i],
                                                strlen(desc->rsp[i])) != 0) {
                                printf("%s: wrong response: %s\n", __func__,
                                                        (char *)rsp_buf);
                                result = AT_WRONG_RSP;
                                /* response is wrong may be an error, lets drain
                                 * uart buffer for this command before
                                 * it exits
                                 */
                                continue;
                        } else {
                                if (i == 0) {
                                        uint16_t count = strlen(desc->rsp[i]);
                                        s_id = (*(char *)(rsp_buf + count)) - '0';
                                }
                        }

                } else
                        result = AT_FAILURE;
        }
        CHECK_SUCCESS(result, AT_SUCCESS, -1);

        /* Now make remote connection */
        desc = &tcp_comm[TCP_CONN];
        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm, s_id, host, port);
        result = __at_comm_send_and_wait_rsp(temp_comm, strlen(temp_comm),
                                                        desc->rsp_timeout);
        CHECK_SUCCESS(result, AT_SUCCESS, -1);

        /* Parse the response */
        result = AT_SUCCESS;
        for(; i < (sizeof(desc->rsp)/sizeof(*(desc->rsp))); i++) {
                if (!desc->rsp[i])
                        continue;
                read_bytes = uart_line_avail(rsp_header, rsp_trailer);
                if (read_bytes <= 0) {
                        printf("%s: comm: %s failed\n", __func__, comm);
                        result = AT_FAILURE;
                        continue;
                }
                uint8_t rsp_buf[read_bytes + 1];
                rsp_buf[read_bytes] = 0x0;
                if (uart_read(rsp_buf, read_bytes) != UART_READ_ERR) {
                        if (strncmp((char *)rsp_buf, desc->rsp[i],
                                                strlen(desc->rsp[i])) != 0) {
                                printf("%s: wrong response: %s\n", __func__,
                                                        (char *)rsp_buf);
                                result = AT_WRONG_RSP;
                                /* response is wrong may be an error, lets drain
                                 * uart buffer for this command before
                                 * it exits
                                 */
                                continue;
                        }

                } else
                        result = AT_FAILURE;
        }
        CHECK_SUCCESS(result, AT_SUCCESS, -1);

        state |= TCP_CONNECTED;
        printf("%s: socket:%d created\n", __func__, s_id);
        return s_id;
}

int at_tcp_connect(const char *host, int port) {
        __at_process_urc();
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

int at_tcp_send(int s_id, const unsigned char *buf, size_t len) {
        uint16_t result;
        uint16_t read_bytes;
        uint16_t wr_bytes = 0;

        const char *write_prompt = "\r\n@";
        __at_process_urc();

        if (len < MAX_TCP_SEND_BYTES)
                return -1;
        if (state & TCP_CONNECTED != TCP_CONNECTED) {
                printf("ATS not connected\n", );
                return -1;
        }

        at_command_desc *desc = &tcp_comm[TCP_SEND];

        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm, s_id, len);

        result = __at_comm_send_and_wait_rsp(temp_comm, strlen(temp_comm), 20);
        CHECK_SUCCESS(result, AT_SUCCESS, -1);

        read_bytes = uart_rx_available();
        if (read_bytes == 0) {
                printf("%s: comm: %s failed\n", __func__, temp_comm);
                return -1;
        }
        uint8_t rsp_buf[read_bytes + 1];
        rsp_buf[read_bytes] = 0x0;
        if (uart_read(rsp_buf, read_bytes) == UART_READ_ERR) {
                printf("%s: comm: %s failed\n", __func__, temp_comm);
                return -1;
        }

        if (strncmp(rsp_buf, write_prompt, strlen(write_prompt)) != 0) {
                printf("%s: comm: %s failed\n", __func__, temp_comm);
                return -1;
        }
        /* recommeneded in datasheet */
        HAL_Delay(50);

        state |= TCP_WRITE;
        int i = 0;
        for (; i < len && ((state & TCP_CONNECTED) == TCP_CONNECTED); i++)
                __at_uart_write(((uint8_t *)buf + i), 1);

        if (state & TCP_CONNECTED != TCP_CONNECTED) {
                //Dipen:FIXME simulate this scenario in test cases which is to terminate connection
                // in a middle of write
                state &= ~TCP_WRITE;
                return -1;
        }

        result = __at_wait_for_rsp(desc->rsp_timeout);

        state &= ~TCP_WRITE;
        CHECK_SUCCESS(result, AT_SUCCESS, -1);

        /* Parse the response for session or socket id and number of write */
        for(; i < (sizeof(desc->rsp)/sizeof(*(desc->rsp))); i++) {
                if (!desc->rsp[i])
                        continue;
                read_bytes = uart_line_avail(rsp_header, rsp_trailer);
                if (read_bytes <= 0) {
                        printf("%s: comm: %s failed\n", __func__, comm);
                        result = AT_FAILURE;
                        continue;
                }
                uint8_t rsp_buf[read_bytes + 1];
                rsp_buf[read_bytes] = 0x0;
                if (uart_read(rsp_buf, read_bytes) != UART_READ_ERR) {
                        if (strncmp((char *)rsp_buf, desc->rsp[i],
                                                strlen(desc->rsp[i])) != 0) {
                                printf("%s: wrong response: %s\n", __func__,
                                                        (char *)rsp_buf);
                                result = AT_WRONG_RSP;
                                /* response is wrong may be an error, lets drain
                                 * uart buffer for this command before
                                 * it exits
                                 */
                                continue;
                        } else {
                                if (i == 0) {
                                        uint16_t count = strlen(desc->rsp[i]);
                                        int s_temp =
                                        (*(char *)(rsp_buf + count)) - '0';
                                        if (s_temp != s_id) {
                                                printf("%s: wrong s_id :%d\n",
                                                        __func__, s_temp);
                                                result = AT_FAILURE;
                                                continue;
                                        }
                                        count += 2;
                                        uint32_t find_end =
                                                __at_find_end(
                                                        (char *)rsp_buf + count,
                                                        '\r');

                                        if (find_end != 0) {
                                                wr_bytes =
                                                __at_convert_to_decimal(
                                                        (char *)rsp_buf + count,
                                                        find_end);
                                                printf("%s: Write Bytes: %d\n",
                                                        __func__, wr_bytes);
                                        } else {
                                                result = AT_FAILURE;
                                                continue;
                                        }
                                }
                        }

                } else
                        result = AT_FAILURE;
        }
        CHECK_SUCCESS(result, AT_SUCCESS, -1);
        return wr_bytes;
}

static int __at_parse_rcv_rsp(uint16_t start, uint8_t *rsp_buf, uint8_t *buf,
                                size_t len) {
        int num_read;
        int s_temp = (*(rsp_buf + start)) - '0';
        if (s_temp != s_id) {
                printf("%s: wrong s_id :%d\n", __func__, s_temp);
                return -1;
        }
        start += 2;
        uint32_t find_end = __at_find_end((char *)rsp_buf + start, ',');
        if (find_end != 0) {
                num_read = __at_convert_to_decimal((char *)rsp_buf + start,
                                                find_end);
                printf("%s: Read Bytes: %d\n", __func__, num_read);
        } else
                return -1;
        start = start + find_end + 2;
        memcpy(buf, rsp_buf + start, num_read);
        start += 3;
        const char *ok = "\r\nOK\r\n";
        if (strncmp(rsp_buf, ok, strlen(ok)) != 0) {
                printf("%s: rare non ok\n", __func__);
        }
        return num_read;
}

bool at_read_available() {
        return (state & TCP_READ == TCP_READ);
}

int at_tcp_recv_timeout(int s_id, unsigned char *buf,
        size_t len, uint32_t timeout) {

        __at_process_urc();

        uint8_t result = AT_SUCCESS;
        int r_bytes = 0;

        at_command_desc *desc = &tcp_comm[TCP_RCV];
        if (state & TCP_CONNECTED != TCP_CONNECTED) {
                printf("ATRT_WRNG_STATE:%u\n", (uint32_t)state);
                //Dipen: wot to return?
                return -1;
        }
        /*Dipen: move this to net layer
        uint32_t start = HAL_GetTick();

        while (state & TCP_READ != TCP_READ) {
                if (HAL_GetTick() - start > timeout) {
                        result = AT_RSP_TIMEOUT;
                        break;
                }
        }
        CHECK_SUCCESS(result, AT_SUCCESS, -1);
        */

        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm, s_id, len);

        result = __at_comm_send_and_wait_rsp(temp_comm, strlen(temp_comm),
                                                desc->rsp_timeout);
        CHECK_SUCCESS(result, AT_SUCCESS, -1);

        read_bytes = uart_rx_available();
        if (read_bytes <= 0) {
                printf("%s: comm: %s failed\n", __func__, comm);
                return -1;
        }
        uint8_t rsp_buf[read_bytes];
        if (uart_read(rsp_buf, read_bytes) != UART_READ_ERR) {
                if (strncmp((char *)rsp_buf, desc->rsp[0],
                                        strlen(desc->rsp[0])) != 0) {
                        result = AT_WRONG_RSP;
                        /* response is wrong may be an error, lets drain
                         * uart buffer for this command before
                         * it exits
                         */
                } else {
                        uint16_t start = strlen(desc->rsp[0]);
                        r_bytes = __at_parse_rcv_rsp(start, rsp_buf, buf);
                }

        } else
                result = AT_FAILURE;
        CHECK_SUCCESS(result, AT_SUCCESS, -1);
        return r_bytes;
}

void at_tcp_close(int s_id) {
        __at_process_urc();
        at_command_desc *desc = &tcp_comm[TCP_CLOSE];
        if (state & TCP_CONNECTED != TCP_CONNECTED) {
                printf("ATC_ALRDY_CLOSED\n");
                return;
        }
        if (s_id < 0)
                return;

        char temp_comm[TEMP_COMM_LIMIT];
        snprintf(temp_comm, TEMP_COMM_LIMIT, desc->comm, s_id);
        result = __at_comm_send_and_wait_rsp(temp_comm, strlen(temp_comm),
                                                        desc->rsp_timeout);
        if (result != AT_SUCCESS)
                return;

        result = AT_SUCCESS;
        for(; i < (sizeof(desc->rsp)/sizeof(*(desc->rsp))); i++) {
                if (!desc->rsp[i])
                        continue;
                read_bytes = uart_line_avail(rsp_header, rsp_trailer);
                if (read_bytes <= 0) {
                        printf("%s: comm: %s failed\n", __func__, comm);
                        result = AT_FAILURE;
                        continue;
                }
                uint8_t rsp_buf[read_bytes + 1];
                rsp_buf[read_bytes] = 0x0;
                if (uart_read(rsp_buf, read_bytes) != UART_READ_ERR) {
                        if (strncmp((char *)rsp_buf, desc->rsp[i],
                                                strlen(desc->rsp[i])) != 0) {
                                printf("%s: wrong response: %s\n", __func__,
                                                        (char *)rsp_buf);
                                result = AT_WRONG_RSP;
                                /* response is wrong may be an error, lets drain
                                 * uart buffer for this command before
                                 * it exits
                                 */
                                continue;
                        }

                } else
                        result = AT_FAILURE;
        }
        if (result != AT_SUCCESS)
                return;
        state &= ~TCP_CONNECTED;
        return;
}
