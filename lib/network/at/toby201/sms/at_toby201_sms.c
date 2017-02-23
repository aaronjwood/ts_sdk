/* Copyright (C) 2017 Verizon. All rights reserved. */
#include <stdio.h>
#include <string.h>

#include "at_sms.h"
#include "at_core.h"
#include "at_toby201_sms_command.h"
#include "platform.h"

#define MAX_TRIES_MODEM_CONFIG	3
#define NET_REG_TIMEOUT_SEC	180000
#define CTRL_Z			0x1A
#define ESC			0x1B
#define OUT_PDU_OFFSET		0x02

static at_sms_cb sms_rx_cb;			/* Receive callback */
static at_msg_t msg;				/* Stores SMS segment */

static char sim_num[ADDR_SZ + 1];		/* Number associated with SIM */

static char addr[ADDR_SZ + 1];			/* Null terminated address */
static uint8_t buf[MAX_DATA_SZ];		/* Buffer for data received */
static char in_pdu[MAX_IN_PDU_SZ];		/* Max. length of incoming PDU */

/*
 * Max. length of outgoing PDU.
 * Account for the NULL (0x00) and CTRL+Z (0x1A) characters at the end and "00"
 * (i.e. 0x30 0x30) in the beginning to use the stored SMSC address.
 */
static char out_pdu[2 + MAX_OUT_PDU_SZ + 2];

static void uart_cb(void)
{
	DEBUG_V0("%s: URC from callback\n", __func__);
	if (!at_core_is_proc_rsp() && !at_core_is_proc_urc())
		at_core_process_urc(false);
}

/* Process the received SMS URC */
static at_ret_code process_ucmt_urc(const char *urc)
{
	uint8_t len = strlen(at_urcs[UCMT_URC]);
	if (strncmp(urc, at_urcs[UCMT_URC], len) != 0)
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

	if (msg_len > MAX_IN_PDU_SZ) {
		DEBUG_V0("%s: Unlikely - Oversized incoming PDU (%u, %u)\n",
				__func__, msg_len, MAX_IN_PDU_SZ);
		return AT_FAILURE;
	}

	buf_sz av_len = at_core_rx_available();
	if (msg_len > av_len) {
		DEBUG_V0("%s: Unlikely - Not enough bytes (%u, %u)\n",
				__func__, msg_len, av_len);
		return AT_FAILURE;
	}

	at_core_read((uint8_t *)in_pdu, msg_len);
	if (!smscodec_decode(msg_len, in_pdu, &msg)) {
		DEBUG_V0("%s: Unlikely - Failed to decode PDU\n", __func__);
		return AT_FAILURE;
	}

	if (sms_rx_cb)
		sms_rx_cb(&msg);

	at_core_clear_rx();
	return AT_SUCCESS;
}

static at_ret_code process_ims_urc(const char *urc)
{
	uint8_t len = strlen(at_urcs[IMS_STAT_URC]);
	if (strncmp(urc, at_urcs[IMS_STAT_URC], len) != 0)
		return AT_FAILURE;

	uint8_t net_stat = urc[len] - '0';
	if (net_stat != 1)
		DEBUG_V0("%s: IMS registration lost\n", __func__);
	else
		DEBUG_V0("%s: Registered to IMS network\n", __func__);

	return AT_SUCCESS;
}

static void urc_cb(const char *urc)
{
	at_ret_code res = process_ims_urc(urc);
	if (res == AT_SUCCESS)
		return;

	res = process_ucmt_urc(urc);
}

static inline at_ret_code check_network_registration(void)
{
	if (!at_core_query_netw_reg())
		return AT_FAILURE;

	/* If not registered to IMS after timeout, modem needs to be restarted */
	if (at_core_wcmd(&mod_netw_cmd[IMS_REG_QUERY], true) != AT_SUCCESS)
		return AT_RECHECK_MODEM;

	return AT_SUCCESS;
}

static at_ret_code config_modem_for_sms(void)
{
	at_ret_code res = AT_FAILURE;

	/* Enable IMS Registration URC */
	res = at_core_wcmd(&mod_netw_cmd[IMS_REG_URC_SET], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);

	/* Set the MNO configuration for Verizon */
	res = at_core_wcmd(&mod_netw_cmd[MNO_CONF_QUERY], true);
	if (res != AT_SUCCESS) {
		res = at_core_wcmd(&mod_netw_cmd[MNO_CONF_SET], true);
		CHECK_SUCCESS(res, AT_SUCCESS, res);
		return AT_RECHECK_MODEM;
	}

	/* Enable automatic timezone update */
	res = at_core_wcmd(&mod_netw_cmd[AUTO_TIME_ZONE_QUERY], true);
	if (res != AT_SUCCESS) {
		res = at_core_wcmd(&mod_netw_cmd[AUTO_TIME_ZONE_SET], true);
		CHECK_SUCCESS(res, AT_SUCCESS, res);
		return AT_RECHECK_MODEM;
	}

	/* Check network registration */
	uint32_t start = platform_get_tick_ms();
	uint32_t end = start;
	do {
		DEBUG_V0("%s: Rechecking network registration\n", __func__);
		res = check_network_registration();
		end = platform_get_tick_ms();
		if (end - start > NET_REG_TIMEOUT_SEC) {
			DEBUG_V0("%s: Network registration timeout\n", __func__);
			break;
		}
		platform_delay(CHECK_MODEM_DELAY);
	} while(res != AT_SUCCESS);

	if (res != AT_SUCCESS) {
		DEBUG_V0("%s: Network registration failed\n", __func__);
		return res;
	}

	/* Set outgoing SMS format to 3GPP */
	res = at_core_wcmd(&sms_cmd[SMS_SET_SMS_FORMAT_3GPP], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);

	/* Enable Phase 2+ features */
	res = at_core_wcmd(&sms_cmd[SMS_SET_CSMS], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);

	/* Set CNMI */
	res = at_core_wcmd(&sms_cmd[SMS_SET_CNMI], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);

	/* Enter PDU mode */
	res = at_core_wcmd(&sms_cmd[SMS_ENTER_PDU_MODE], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);

	return res;
}

bool at_init(void)
{
	msg.len = 0;
	msg.buf = buf;
	msg.addr = addr;

	if (!at_core_init(uart_cb, urc_cb))
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
	char cmd[13];	/* Enough to store "AT+CMGS=xxx\r\0" */

	/* Form and issue the command to send the PDU over the network */
	snprintf(cmd, sizeof(cmd), sms_cmd[SMS_SEND].comm_sketch, pdu_strlen / 2);
	sms_cmd[SMS_SEND].comm = cmd;
	at_ret_code res = at_core_wcmd(&sms_cmd[SMS_SEND], false);
	if (res != AT_SUCCESS) {
		uint8_t esc = ESC;
		at_core_write(&esc, 1);
		return false;
	}

	/* Enter the PDU bytes followed by Ctrl+Z (0x1A) */
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

bool at_sms_retrieve_num(char *num)
{
	at_ret_code res = at_core_wcmd(&mod_netw_cmd[SIM_NUM], true);
	CHECK_SUCCESS(res, AT_SUCCESS, res);
	strncpy(num, sim_num, strlen(sim_num) + 1);
	return true;
}

static void parse_num(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	/* Retrieve the second quoted string from the response */
	const char *start = strchr(rcv_rsp, ',') + 2;
	size_t span = strcspn(start, "\"");
	if (span > ADDR_SZ || span > rcv_rsp_len)
		return;
	strncpy(sim_num, start, span);
	sim_num[span] = '\0';
}
