/* Copyright (C) 2017 Verizon. All rights reserved. */

#include <stdio.h>
#include <string.h>
#include "at_core.h"
#include "at_modem.h"
#include "sys.h"
#include "gpio_hal.h"
#include "ts_sdk_board_config.h"

/*
 * Delay between successive commands in milisecond, datasheet recommends atleast
 * 20mS
 */
#define MODEM_RESET_DELAY		25000	/* In milli seconds */
#define RESET_PULSE_WIDTH_MS		3000	/* Toby-L2 data sheet Section 4.2.9 */
#define CHECK_MODEM_DELAY		5000	/* In ms, polling for modem */

enum modem_core_commands {
	MODEM_OK,	/* Simple modem check command, i.e. AT */
	ECHO_OFF,	/* Turn off echoing AT commands given to the modem */
	CME_CONF,	/* Change the error output format to numerical */
	MODEM_RESET,	/* Command to reset the modem */
	SIM_READY,	/* Check if the SIM is present */
	EPS_REG_QUERY,	/* Query EPS registration */
	EPS_URC_SET,	/* Set the EPS registration URC */
	ExPS_REG_QUERY,	/* Query extended packet switched network registration */
	ExPS_URC_SET,	/* Set the extended packet switched network reg URC */
	MODEM_CORE_END	/* End-of-commands marker */
};

typedef enum at_core_urc {
	EPS_STAT_URC,
	ExPS_STAT_URC,
	NUM_URCS
} at_core_urc;

static const char *at_urcs[NUM_URCS] = {
	[EPS_STAT_URC] = "\r\n+CEREG: ",
	[ExPS_STAT_URC] = "\r\n+UREG: "
};

static const at_command_desc modem_core[MODEM_CORE_END] = {
        [MODEM_OK] = {
                .comm = "at\r",
                .rsp_desc = {
                        {
                                .rsp = "at\r\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 100
        },
        [ECHO_OFF] = {
                .comm = "ate0\r",
                .rsp_desc = {
                        {
                                .rsp = "ate0\r\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = "\r\nERROR\r\n",
                .comm_timeout = 100
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
                .comm_timeout = 100
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
                .comm_timeout = 100
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
                .comm_timeout = 100
        },
        [ExPS_REG_QUERY] = {
                .comm = "at+ureg?\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\n+UREG: 1,7\r\n",
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
                .comm_timeout = 100
        },
        [ExPS_URC_SET] = {
                .comm = "at+ureg=1\r",
                .rsp_desc = {
                        {
                                .rsp = "\r\nOK\r\n",
                                .rsp_handler = NULL,
                                .data = NULL
                        }
                },
                .err = NULL,
                .comm_timeout = 100
        },
        [MODEM_RESET] = {
                .comm = "at+cfun=16\r",
		.rsp_desc = {
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
                .err = NULL,
                .comm_timeout = 7000
        }
};

void at_modem_hw_reset(void)
{
	gpio_write(MODEM_HW_RESET_PIN, PIN_LOW);
	sys_delay(RESET_PULSE_WIDTH_MS);
	gpio_write(MODEM_HW_RESET_PIN, PIN_HIGH);
}

bool at_modem_sw_reset(void)
{
	return at_core_wcmd(&modem_core[MODEM_RESET], false) == AT_SUCCESS ?
		true : false;
}

bool at_modem_configure(void)
{
	/* Make sure the modem is ready to accept commands */
	uint32_t start = sys_get_tick_ms();
	uint32_t end;
	at_ret_code result = AT_FAILURE;
	while (result != AT_SUCCESS) {
		end = sys_get_tick_ms();
		if ((end - start) > MODEM_RESET_DELAY) {
			DEBUG_V0("%s: timed out\n", __func__);
			return result;
		}
		result = at_core_wcmd(&modem_core[MODEM_OK], false);
		sys_delay(CHECK_MODEM_DELAY);
	}

	/* Switch off TX echo */
	result = at_core_wcmd(&modem_core[ECHO_OFF], false);
	CHECK_SUCCESS(result, AT_SUCCESS, false);

	/* Check if the SIM card is present */
	result = at_core_wcmd(&modem_core[SIM_READY], true);
	CHECK_SUCCESS(result, AT_SUCCESS, false);

	/* Set error format to numerical */
	result = at_core_wcmd(&modem_core[CME_CONF], true);
	CHECK_SUCCESS(result, AT_SUCCESS, false);

	/* Enable the Extended Packet Switched network registration URC */
	result = at_core_wcmd(&modem_core[ExPS_URC_SET], true);
	CHECK_SUCCESS(result, AT_SUCCESS, false);

	/* Enable the EPS network registration URC */
	result = at_core_wcmd(&modem_core[EPS_URC_SET], true);
	CHECK_SUCCESS(result, AT_SUCCESS, false);

	return true;
}

bool at_modem_query_network(void)
{
	at_ret_code res = at_core_wcmd(&modem_core[EPS_REG_QUERY], true);
	if (res != AT_SUCCESS)
		return false;

	res = at_core_wcmd(&modem_core[ExPS_REG_QUERY], true);
	if (res != AT_SUCCESS)
		return false;

	return true;
}

static bool process_network_urc(const char *urc, at_core_urc u_code)
{
	if (strncmp(urc, at_urcs[u_code], strlen(at_urcs[u_code])) != 0)
		return false;

	uint8_t last_char = strlen(at_urcs[u_code]);
	uint8_t net_stat = urc[last_char] - '0';
	DEBUG_V0("%s: Net stat (%u): %u\n", __func__, u_code, net_stat);
	switch (u_code) {
	case EPS_STAT_URC:
		if (net_stat == 0 || net_stat == 3 || net_stat == 4)
			DEBUG_V0("%s: EPS network reg lost\n", __func__);
		else
			DEBUG_V0("%s: Registered to EPS network\n", __func__);
		break;
	case ExPS_STAT_URC:
		if (net_stat == 0)
			DEBUG_V0("%s: Extended PS network reg lost\n", __func__);
		else
			DEBUG_V0("%s: Registered to extended PS network\n", __func__);
		break;
	default:
		return false;
	}
	return true;
}

bool at_modem_process_urc(const char *urc)
{
	if (process_network_urc(urc, EPS_STAT_URC))
		return true;

	if (process_network_urc(urc, ExPS_STAT_URC))
		return true;

	return false;
}

bool at_modem_init(void)
{
	gpio_config_t reset_pins;
	reset_pins.dir = OUTPUT;
	reset_pins.pull_mode = OD_NO_PULL;
	reset_pins.speed = SPEED_HIGH;
	if (gpio_init(MODEM_HW_RESET_PIN, &reset_pins)) {
		gpio_write(MODEM_HW_RESET_PIN, PIN_HIGH);
		return true;
	}
	return false;
}
