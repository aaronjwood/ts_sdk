/* Copyright (C) 2017 Verizon. All rights reserved. */
#include <stdio.h>

#include "at_sms.h"
#include "at_core.h"

#define MAX_TRIES_MODEM_CONFIG	2

static void uart_cb(void)
{
	/* Code */
}

static void urc_cb(const char *urc)
{
	/* Code */
}

static at_ret_code config_modem_for_sms(uint8_t try_num)
{
	at_ret_code res = AT_SUCCESS;
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
		res = config_modem_for_sms(num_tries);
		if (res == AT_SUCCESS)
			break;
		if (res == AT_RECHECK_MODEM)
			DEBUG_V0("%s: Attempt %u at configuring modem failed\n",
					__func__, num_tries);
		num_tries++;
	}

	if (res != AT_SUCCESS)
		return false;

	DEBUG_V0("Modem configured\n");
	return true;
}
