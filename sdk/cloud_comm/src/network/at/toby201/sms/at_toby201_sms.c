/* Copyright (C) 2017 Verizon. All rights reserved. */
#include <stdio.h>
#include <string.h>

#include "at_sms.h"
#include "at_core.h"
#include "at_toby201_sms_command.h"
#include "sys.h"

/*
 * Delay between successive commands in milisecond, datasheet recommends atleast
 * 20mS
 */
#define AT_COMM_DELAY_MS		20
#define CHECK_MODEM_DELAY		5000	/* In ms, polling for modem */

#define MAX_TRIES_MODEM_CONFIG	3		/* Retries at configuring modem */
#define NET_REG_TIMEOUT_SEC	180000		/* Network registration timeout */
#define CTRL_Z			0x1A		/* Ctrl + Z character code */
#define OUT_PDU_OFFSET		0x02		/* Offset where actual PDU begins */

static at_sms_cb sms_rx_cb;			/* Receive callback */
static at_msg_t msg;				/* Stores SMS segment */

static char addr[ADDR_SZ + 1];			/* Null terminated address */
static uint8_t buf[MAX_DATA_SZ];		/* Buffer for data received */
static char in_pdu[MAX_IN_PDU_SZ];		/* Max. length of incoming PDU */

/*
 * Max. length of outgoing PDU.
 * Account for the CTRL+Z (0x1A) and NULL (0x00) characters at the end and "00"
 * (i.e. 0x30 0x30) in the beginning to use the stored SMSC address.
 */
static char out_pdu[2 + MAX_OUT_PDU_SZ + 2];

/* Aids in retrieving the PDU from the URC */
static volatile bool recv_pdu_in_progress;
static volatile buf_sz wanted_bytes;

static void attempt_pdu_retrieval(void)
{
	if (recv_pdu_in_progress) {
		if (at_core_rx_available() >= wanted_bytes) {
			/* Read the URC data (PDU string) and decode it into an
			 * SMS segment */
			at_core_read((uint8_t *)in_pdu, wanted_bytes);
			if (!smscodec_decode(wanted_bytes, in_pdu, &msg)) {
				DEBUG_V0("%s: Unlikely - Failed to decode PDU\n",
						__func__);
				return;
			}

			if (sms_rx_cb)
				sms_rx_cb(&msg);

			/* Read the trailing CRLF */
			at_core_read((uint8_t *)in_pdu, 2);

			wanted_bytes = 0;
			recv_pdu_in_progress = false;
		}
	}
}

static void uart_cb(void)
{
	DEBUG_V0("%s: URC from callback\n", __func__);

	attempt_pdu_retrieval();

	if (!at_core_is_proc_rsp() && !at_core_is_proc_urc())
		at_core_process_urc(false);
}

/* Process the received SMS URC */
static at_ret_code process_cmt_urc(const char *urc)
{
	uint8_t len = strlen(at_urcs[CMT_URC]);
	if (strncmp(urc, at_urcs[CMT_URC], len) != 0)
		return AT_FAILURE;

	/* Get length of the PDU stream in bytes from the URC header */
	buf_sz msg_len = 0;
	uint8_t i = len;
	while (urc[i] != '\r') {
		if (urc[i] < '0' && urc[i] > '9')
			return AT_FAILURE;
		msg_len *= 10;
		msg_len += (urc[i] - '0');
		i++;
	}

	msg_len *= 2;		/* 2 hexadecimal digits make up 1 byte */

	/* The length field does not include the size of the service center
	 * address since it is not part of the PDU.
	 */
	if (msg_len + SMSC_ADDR_SZ > MAX_IN_PDU_SZ) {
		DEBUG_V0("%s: Unlikely - Oversized incoming PDU (%u, %u)\n",
				__func__, msg_len, MAX_IN_PDU_SZ);
		return AT_FAILURE;
	}

	recv_pdu_in_progress = true;
	wanted_bytes = msg_len + SMSC_ADDR_SZ;

	attempt_pdu_retrieval();

	return AT_SUCCESS;
}

static at_ret_code process_net_urc(const char *urc)
{
	uint8_t len = strlen(at_urcs[NET_STAT_URC]);
	if (strncmp(urc, at_urcs[NET_STAT_URC], len) != 0)
		return AT_FAILURE;

	uint8_t net_stat = urc[len] - '0';
	if (net_stat != NET_STAT_REG_CODE)
		DEBUG_V0("%s: Network registration lost\n", __func__);
	else
		DEBUG_V0("%s: Registered to network\n", __func__);

	return AT_SUCCESS;
}

static void urc_cb(const char *urc)
{
	at_ret_code res = process_net_urc(urc);
	if (res == AT_SUCCESS)
		return;

	res = process_cmt_urc(urc);
}

static at_ret_code check_network_registration(void)
{
	if (!at_core_query_netw_reg())
		return AT_FAILURE;

	/* Restart modem if not registered to the network after timeout */
	if (at_core_wcmd(&mod_netw_cmd[NET_REG_QUERY], true) != AT_SUCCESS)
		return AT_RECHECK_MODEM;

	return AT_SUCCESS;
}

static at_ret_code config_modem_for_sms(void)
{
	at_ret_code res = AT_FAILURE;

	/* Enable Network Registration URC */
	res = at_core_wcmd(&mod_netw_cmd[NET_REG_URC_SET], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);

	/* Set the MNO configuration for Verizon */
	res = at_core_wcmd(&mod_netw_cmd[MNO_CONF_QUERY], true);
	if (res != AT_SUCCESS) {
		res = at_core_wcmd(&mod_netw_cmd[MNO_CONF_SET], true);
		CHECK_SUCCESS(res, AT_SUCCESS, res);
		return AT_RECHECK_MODEM;
	}

	/* Check network registration */
	uint32_t start = sys_get_tick_ms();
	uint32_t end = start;
	do {
		DEBUG_V0("%s: Rechecking network registration\n", __func__);
		res = check_network_registration();
		if (res == AT_SUCCESS)
			break;
		sys_delay(CHECK_MODEM_DELAY);
		end = sys_get_tick_ms();
	} while(end - start < NET_REG_TIMEOUT_SEC);

	if (end - start >= NET_REG_TIMEOUT_SEC) {
		DEBUG_V0("%s: Network registration timeout\n", __func__);
		return res;
	}

	/* Enable Phase 2+ features */
	res = at_core_wcmd(&sms_cmd[SMS_SET_CSMS], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);

	/* Set CNMI */
	res = at_core_wcmd(&sms_cmd[SMS_SET_CNMI], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);

	/* Enter PDU mode */
	res = at_core_wcmd(&sms_cmd[SMS_ENTER_PDU_MODE], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);

	/* Delete all stored SMSes */
	res = at_core_wcmd(&sms_cmd[SMS_DEL_ALL_MSG], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);

	return res;
}

bool at_init(void)
{
	msg.len = 0;
	msg.buf = buf;
	msg.addr = addr;

	if (!at_core_init(uart_cb, urc_cb, AT_COMM_DELAY_MS))
		return false;

	at_ret_code res = at_core_modem_reset();
	if (res != AT_SUCCESS) {
		printf("%s: Modem reset failed\n", __func__);
		return false;
	}

	uint8_t num_tries = 0;
	while (num_tries < MAX_TRIES_MODEM_CONFIG) {
		res = config_modem_for_sms();
		if (res == AT_SUCCESS)
			break;
		DEBUG_V0("%s: Attempt %u at configuring modem failed\n",
					__func__, num_tries);
		if (res == AT_RECHECK_MODEM) {
			at_ret_code reset_res = at_core_modem_reset();
			if (reset_res != AT_SUCCESS) {
				printf("%s: Modem reset failed\n", __func__);
				return false;
			}
		}
		num_tries++;
	}

	if (res != AT_SUCCESS)
		return false;

	DEBUG_V0("Modem configured\n");
	return true;
}

bool at_sms_send(const at_msg_t *sms_seg)
{
	/* Encode the message into a PDU string and retrieve its length */
	uint16_t pdu_strlen = smscodec_encode(sms_seg, out_pdu + OUT_PDU_OFFSET);
	out_pdu[0] = '0';
	out_pdu[1] = '0';
	out_pdu[OUT_PDU_OFFSET + pdu_strlen] = CTRL_Z;
	out_pdu[OUT_PDU_OFFSET + pdu_strlen + 1] = '\0';

	/* Issue the command to send the PDU over the network */
	char cmd[13];	/* Enough to store "AT+CMGS=xxx\r\0" */
	snprintf(cmd, sizeof(cmd), sms_cmd[SMS_SEND].comm_sketch, pdu_strlen / 2);
	sms_cmd[SMS_SEND].comm = cmd;
	at_ret_code res = at_core_wcmd(&sms_cmd[SMS_SEND], false);
	CHECK_SUCCESS(res, AT_SUCCESS, false);

	/* Enter the PDU bytes followed by Ctrl+Z */
	sms_cmd[SMS_SEND_DATA].comm = out_pdu;
	res = at_core_wcmd(&sms_cmd[SMS_SEND_DATA], true);
	CHECK_SUCCESS(res, AT_SUCCESS, false);
	return true;
}

void at_sms_set_rcv_cb(at_sms_cb cb)
{
	if (cb != NULL)
		sms_rx_cb = cb;
	return;
}

bool at_sms_ack(void)
{
	return (at_core_wcmd(&sms_cmd[SMS_SEND_ACK], true) == AT_SUCCESS) ?
		true : false;
}

bool at_sms_nack(void)
{
	return (at_core_wcmd(&sms_cmd[SMS_SEND_NACK], true) == AT_SUCCESS) ?
		true : false;
}
