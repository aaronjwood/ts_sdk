/* Copyright (C) 2017 Verizon. All rights reserved. */
#include <stdio.h>
#include <string.h>

#include "at_sms.h"
#include "at_core.h"
#include "at_toby201_sms_command.h"
#include "platform.h"
#include "smscodec.h"

#define MAX_TRIES_MODEM_CONFIG	3
#define NET_REG_TIMEOUT_SEC	180000

static void uart_cb(void)
{
	/* Stub for now */
}

static at_sms_cb sms_rx_cb;

static at_ret_code process_ucmt_urc(const char *urc, at_urc u_code)
{
	return AT_SUCCESS;
}

static at_ret_code process_ims_urc(const char *urc, at_urc u_code)
{
	if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
		return AT_FAILURE;

	if (u_code == IMS_STAT_URC) {
		uint8_t last_char = strlen(at_urcs[u_code]);
		uint8_t net_stat = urc[last_char] - '0';
		if (net_stat != 1)
			DEBUG_V0("%s: IMS registration lost\n", __func__);
	} else
		return AT_FAILURE;

	return AT_SUCCESS;
}

static void urc_cb(const char *urc)
{
	at_ret_code res = process_ims_urc(urc, IMS_STAT_URC);
	if (res == AT_SUCCESS)
		return;

	res = process_ucmt_urc(urc, UCMT_URC);
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

	if (res != AT_SUCCESS)
		DEBUG_V0("%s: Network registration failed\n", __func__);

	return res;
}

bool at_init(void)
{
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
	/* XXX: Stub for now */
	return true;
}

bool at_sms_nack(void)
{
	/* XXX: Stub for now */
	return true;
}
