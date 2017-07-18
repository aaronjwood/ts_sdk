/* Copyright (C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdio.h>
#include "at_core.h"
#include "at_modem.h"
#include "sys.h"
#include "gpio_hal.h"
#include "ts_sdk_modem_config.h"

#define PWR_EN_DELAY_MS			1000
#define RESET_N_DELAY_MS		1000

#define IMEI_LEN			16	/* IMEI length + NULL */
#define SIG_STR_LEN			11	/* Signal strength length + NULL */

#define STABLE_TIMEOUT_MS		5000	/* Wait until modem fully restarts */

enum modem_core_commands {
	MODEM_OK,	/* Simple modem check command, i.e. AT */
	CME_CONF,	/* Change the error output format to numerical */
	MODEM_RESET,	/* Command to reset the modem */
	SIM_READY,	/* Check if the SIM is present */
	EPS_REG_QUERY,	/* Query EPS registration */
	EPS_URC_SET,	/* Set the EPS registration URC */
	IMEI_QUERY,	/* Query the IMEI of the modem */
	SIG_STR_QUERY,	/* Query the signal strength */
	MODEM_CORE_END	/* End-of-commands marker */
};

enum at_urc {
	EPS_STAT_URC,
	SYS_START_URC,
	SIM_WAIT_URC1,
	SIM_WAIT_URC2,
	SIM_READY_URC,
	NUM_URCS
};

static const char *at_urcs[NUM_URCS] = {
	[EPS_STAT_URC] = "\r\n+CEREG: ",
	[SYS_START_URC] = "\r\n+SYSSTART\r\n",
	[SIM_WAIT_URC1] = "\r\n+IMSSTATE: SIMSTORE,WAIT_SIM\r\n",
	[SIM_WAIT_URC2] = "\r\n+IMSSTATE: SIMSTORE,WAIT_STORE\r\n",
	[SIM_READY_URC] = "\r\n+IMSSTATE: SIMSTORE,READY\r\n"
};

static char modem_imei[IMEI_LEN - 1];	/* Store the IMEI */
static uint8_t sig_str;			/* Store numerical signal strength */
static bool sys_res_stable;		/* True when system is stable after restart*/


static void parse_imei(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	sys_delay(5);	/* Precaution to ensure the IMEI has been received */
	/* Skip the CR LF in the beginning of the response */
	rcv_rsp += strlen(stored_rsp);
	memcpy(modem_imei, rcv_rsp, IMEI_LEN - 1);
}

static void parse_sig_str(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	char *rcv_bytes = (char *)rcv_rsp + strlen(stored_rsp);
	sys_delay(10);	/* Precaution to ensure the signal strength has been received */
	uint8_t i = 0;
	sig_str = 0;
	while (rcv_bytes[i] != ',') {
		if (rcv_bytes[i] < '0' || rcv_bytes[i] > '9') {
			sig_str = 0xFF;
			return;
		}
		sig_str = sig_str * 10 + rcv_bytes[i] - '0';
		i++;
	}
}

/*
 * The modem datasheet does not provide any command specific timeout values.
 * The reference firmware uses 10s as timeout. Use a stricter 5s here.
 */
static const at_command_desc modem_core[MODEM_CORE_END] = {
	[MODEM_OK] = {
		.comm = "at\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = NULL,
		.comm_timeout = 5000
	},
	[CME_CONF] = {
		.comm = "at+cmee=1\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = NULL,
		.comm_timeout = 5000
	},
        [MODEM_RESET] = {
                .comm = "at^reset\r",
		.rsp_desc = {
			{
				/*
				 * Should actually be +SYSSHDN but the modem
				 * has a tendency to only partially print the
				 * response.
				 */
				.rsp = "\r\n+SY",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
                .err = NULL,
                .comm_timeout = 5000
        },
        [SIM_READY] = {
                .comm = "at+cpin?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+CPIN: READY\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\n+CME ERROR: ",
                .comm_timeout = 15000
        },
        [EPS_REG_QUERY] = {
                .comm = "at+cereg?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+CEREG: 1,1\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        },
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 5000
        },
        [EPS_URC_SET] = {
                .comm = "at+cereg=1\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 5000
        },
	[IMEI_QUERY] = {
		.comm = "at+cgsn\r",
		.rsp_desc = {
			{
				.rsp = "\r\n",
				.rsp_handler = parse_imei,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = NULL,
		.comm_timeout = 100
	},
	[SIG_STR_QUERY] = {
		.comm = "at+csq\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+CSQ: ",
				.rsp_handler = parse_sig_str,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = NULL,
		.comm_timeout = 100
	}
};

static bool process_urc(const char *urc, enum at_urc u_code)
{
	if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
		return false;

	uint8_t last_char = strlen(at_urcs[u_code]);
	uint8_t net_stat = urc[last_char] - '0';
	switch (u_code) {
	case EPS_STAT_URC:
		DEBUG_V0("%s: Net stat (%u): %u\n", __func__, u_code, net_stat);
		if (net_stat == 0 || net_stat == 3 || net_stat == 4)
			DEBUG_V0("%s: EPS network reg lost\n", __func__);
		else
			DEBUG_V0("%s: Registered to EPS network\n", __func__);
		break;
	case SYS_START_URC:
		DEBUG_V0("%s: Power On\n", __func__);
		break;
	case SIM_WAIT_URC1:
		DEBUG_V0("%s: Wait for SIM\n", __func__);
		break;
	case SIM_WAIT_URC2:
		DEBUG_V0("%s: Wait for STORE\n", __func__);
		break;
	case SIM_READY_URC:
		DEBUG_V0("%s: Sys Ready\n", __func__);
		sys_res_stable = true;
		break;
	default:
		return false;
	}
	return true;
}

bool at_modem_process_urc(const char *urc)
{
	if (process_urc(urc, EPS_STAT_URC))
		return true;
	if (process_urc(urc, SYS_START_URC))
		return true;
	if (process_urc(urc, SIM_WAIT_URC1))
		return true;
	if (process_urc(urc, SIM_WAIT_URC2))
		return true;
	if (process_urc(urc, SIM_READY_URC))
		return true;
	return false;
}

bool at_modem_query_network(void)
{
	return (at_core_wcmd(&modem_core[EPS_REG_QUERY], true) != AT_SUCCESS) ?
		false : true;
}

bool at_modem_configure(void)
{
	/* Check if the SIM card is present */
	at_ret_code result = at_core_wcmd(&modem_core[SIM_READY], true);
	CHECK_SUCCESS(result, AT_SUCCESS, false);

	/* Set error format to numerical */
	result = at_core_wcmd(&modem_core[CME_CONF], true);
	CHECK_SUCCESS(result, AT_SUCCESS, false);

	/* Enable the EPS network registration URC */
	result = at_core_wcmd(&modem_core[EPS_URC_SET], true);
	CHECK_SUCCESS(result, AT_SUCCESS, false);

	return true;
}

static bool wait_for_clean_restart(uint32_t timeout_ms)
{
	uint32_t start = sys_get_tick_ms();
	while (!sys_res_stable) {
		uint32_t now = sys_get_tick_ms();
		if (now - start > timeout_ms) {
			DEBUG_V0("%s: timed out waiting for a clean reset\n",
					__func__);
			return false;
		}
	}
	return true;
}

bool at_modem_sw_reset(void)
{
	sys_res_stable = false;
	at_ret_code res = at_core_wcmd(&modem_core[MODEM_RESET], false);
	if (res == AT_SUCCESS) {
		/* Block until modem is stable or timeout */
		if (!wait_for_clean_restart(STABLE_TIMEOUT_MS)) {
			sys_res_stable = true;
			return false;
		}
		return true;
	}
	sys_res_stable = true;
	return false;
}

void at_modem_hw_reset(void)
{
	sys_res_stable = false;
	gpio_write(MODEM_HW_RESET_PIN, PIN_LOW);
	sys_delay(RESET_N_DELAY_MS);
	gpio_write(MODEM_HW_PWREN_PIN, PIN_LOW);
	sys_delay(PWR_EN_DELAY_MS);
	gpio_write(MODEM_HW_PWREN_PIN, PIN_HIGH);
	sys_delay(RESET_N_DELAY_MS);
	gpio_write(MODEM_HW_RESET_PIN, PIN_HIGH);

	/* Block until modem is stable or timeout */
	wait_for_clean_restart(STABLE_TIMEOUT_MS);
}

bool at_modem_init(void)
{
	gpio_config_t lte_pin_config = {
		.dir = OUTPUT,
		.pull_mode = PP_NO_PULL,
		.speed = SPEED_HIGH
	};

	sys_delay(PWR_EN_DELAY_MS);

	/* Power enable pin */
	if (!gpio_init(MODEM_HW_PWREN_PIN, &lte_pin_config))
		return false;

	/* LTE reset pin */
	if (!gpio_init(MODEM_HW_RESET_PIN, &lte_pin_config))
		return false;

	at_modem_hw_reset();

	return true;
}

bool at_modem_get_imei(char *imei)
{
	if (imei == NULL)
		return false;
	memset(imei, 0, IMEI_LEN);

	if (at_core_wcmd(&modem_core[IMEI_QUERY], true) != AT_SUCCESS)
		return false;

	for (uint8_t i = 0; i < IMEI_LEN - 1; i++) {
		if (modem_imei[i] < '0' || modem_imei[i] > '9')
			return false;
		imei[i] = modem_imei[i];
	}
	imei[IMEI_LEN] = 0x00;

	return true;
}

static bool decode_sig_str(uint8_t sig_str, char *ss)
{
	if (sig_str > 99 || (sig_str > 31 && sig_str < 99))
		return false;

	if (sig_str == 0)
		strcpy(ss, "<=-113 dBm");
	else if (sig_str == 99)
		strcpy(ss, "SIGUNKWN");
	else if (sig_str == 31)
		strcpy(ss, ">=-51 dBm");
	else
		snprintf(ss, SIG_STR_LEN - 1, "%d dBm", -113 + sig_str * 2);
	return true;
}

bool at_modem_get_ss(char *ss)
{
	if (ss == NULL)
		return false;
	memset(ss, 0, SIG_STR_LEN);

	if (at_core_wcmd(&modem_core[SIG_STR_QUERY], true) != AT_SUCCESS)
		return false;

	if (!decode_sig_str(sig_str, ss))
		return false;
	return true;
}
