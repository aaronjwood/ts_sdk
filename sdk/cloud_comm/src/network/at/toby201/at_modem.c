/* Copyright (C) 2017 Verizon. All rights reserved. */

#include <stdio.h>
#include <string.h>
#include "at_core.h"
#include "at_modem.h"
#include "sys.h"
#include "gpio_hal.h"
#include "ts_sdk_modem_config.h"
#include "at_tcp.h"

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
	IMEI_QUERY,	/* Query the IMEI of the modem */
	SIG_STR_QUERY,	/* Query the signal strength */
	MODEM_CORE_END	/* End-of-commands marker */
};

enum at_urc {
	EPS_STAT_URC,
	ExPS_STAT_URC,
	NUM_URCS
};

static const char *at_urcs[NUM_URCS] = {
	[EPS_STAT_URC] = "\r\n+CEREG: ",
	[ExPS_STAT_URC] = "\r\n+UREG: "
};

enum modem_query_commands {
	GET_IMEI,	/* Query the IMEI of the modem */
	GET_SIG_STR,	/* Query the signal strength */
	GET_IP_ADDR,	/* Get the IP address */
	GET_ICCID,	/* Get the ICCID */
	GET_TTZ,	/* Get the time and timezone information */
	GET_IMSI,	/* Get the IMSI */
	GET_MOD_INFO,	/* Get the model information */
	GET_MAN_INFO,	/* Get the manufacturer information */
	GET_FWVER,	/* Get the firmware version */
	MODEM_QUERY_END
};

/* Minimum lengths of the resposne buffers including the terminating NULL character */
static const uint8_t buf_len[MODEM_QUERY_END] = {
	[GET_IMEI] = 16,
	[GET_SIG_STR] = 11,
	[GET_IP_ADDR] = 16,
	[GET_ICCID] = 23,
	[GET_TTZ] = 21,
	[GET_IMSI] = 17,
	[GET_MOD_INFO] = 7,	/* XXX: Might change in next release of modem */
	[GET_MAN_INFO] = 23,	/* XXX: Might change in next release of modem */
	[GET_FWVER] = 11	/* XXX: Might change in next release of modem */
};

static void parse_ip_addr(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	char *rcv_bytes = (char *)rcv_rsp + strlen(stored_rsp);
	if (rcv_bytes[0] != ',')
		return;		/* PDP context is probably not active; no IP */
	rcv_bytes += 2;		/* Skip the ',' and the '"' */
	uint8_t i = 0;
	while (rcv_bytes[i] != '"') {
		((char *)data)[i] = rcv_bytes[i];
		i++;
	}
}

static void parse_imei(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	rcv_rsp += strlen(stored_rsp);
	memcpy(data, rcv_rsp, buf_len[GET_IMEI] - 1);
}

static void decode_sig_str(uint8_t sig_str, char *ss)
{
	if (sig_str > 99 || (sig_str > 31 && sig_str < 99))
		return;

	if (sig_str == 0)
		strcpy(ss, "<=-113 dBm");
	else if (sig_str == 99)
		strcpy(ss, "SIGUNKWN");
	else if (sig_str == 31)
		strcpy(ss, ">=-51 dBm");
	else
		snprintf(ss, buf_len[GET_SIG_STR] - 1,
				"%d dBm", -113 + sig_str * 2);
	return;
}

static void parse_sig_str(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	char *rcv_bytes = (char *)rcv_rsp + strlen(stored_rsp);
	uint8_t i = 0;
	uint8_t sig_str = 0;
	while (rcv_bytes[i] != ',') {
		if (rcv_bytes[i] < '0' || rcv_bytes[i] > '9') {
			sig_str = 0xFF;
			return;
		}
		sig_str = sig_str * 10 + rcv_bytes[i] - '0';
		i++;
	}
	decode_sig_str(sig_str, data);
}

static void parse_iccid(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	char *rcv_bytes = (char *)rcv_rsp + strlen(stored_rsp);
	uint8_t i = 0;
	while (rcv_bytes[i] != '\r') {
		((char *)data)[i] = rcv_bytes[i];
		 i++;
	}
}

static void parse_ttz(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	char *rcv_bytes = (char *)rcv_rsp + strlen(stored_rsp);
	uint8_t i = 0;
	while (rcv_bytes[i] != '"') {
		((char *)data)[i] = rcv_bytes[i];
		 i++;
	}
	
}

static void parse_man_info(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	char *rcv_bytes = (char *)rcv_rsp + strlen(stored_rsp);
	uint8_t i = 0;
	while (rcv_bytes[i] != '\r') {
		((char *)data)[i] = rcv_bytes[i];
		i++;
	}
}

static void parse_mod_info(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	char *rcv_bytes = (char *)rcv_rsp + strlen(stored_rsp);
	uint8_t i = 0;
	while (rcv_bytes[i] != '\r') {
		((char *)data)[i] = rcv_bytes[i];
		i++;
	}
}

static void parse_fwver(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	char *rcv_bytes = (char *)rcv_rsp + strlen(stored_rsp);
	uint8_t i = 0;
	while (rcv_bytes[i] != '\r') {
		((char *)data)[i] = rcv_bytes[i];
		i++;
	}
}

static void parse_imsi(void *rcv_rsp, int rcv_rsp_len,
		const char *stored_rsp, void *data)
{
	rcv_rsp += strlen(stored_rsp);
	memcpy(data, rcv_rsp, buf_len[GET_IMSI] - 1);
}

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

static at_command_desc modem_query[MODEM_QUERY_END] = {
	[GET_IMEI] = {
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
	[GET_SIG_STR] = {
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
		.comm_timeout = 500
	},
	[GET_IP_ADDR] = {
		.comm = "at+cgpaddr="MODEM_APN_CTX_ID"\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+CGPADDR: "MODEM_APN_CTX_ID"",
				.rsp_handler = parse_ip_addr,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 500
	},
	[GET_ICCID] = {
		.comm = "at+ccid\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+CCID: ",   
				.rsp_handler = parse_iccid,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 500
	},
	[GET_TTZ] = {
		.comm = "at+cclk?\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+CCLK: \"",
				.rsp_handler = parse_ttz,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 500
	},
	[GET_MOD_INFO] = {
		.comm = "at+cgmm\r",
		.rsp_desc = {
			{
				.rsp = "\r\n",
				.rsp_handler = parse_mod_info,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 100
	},
	[GET_MAN_INFO] = {
		.comm = "at+cgmi\r",
		.rsp_desc = {
			{
				.rsp = "\r\n",
				.rsp_handler = parse_man_info,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 100
	},
	[GET_IMSI] = {
		.comm = "at+cimi\r",
		.rsp_desc = {
			{
				.rsp = "\r\n",
				.rsp_handler = parse_imsi,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 100
	},
	[GET_FWVER] = {
		.comm = "at+cgmr\r",
		.rsp_desc = {
			{
				.rsp = "\r\n",
				.rsp_handler = parse_fwver,
				.data = NULL
			},
			{
				.rsp = "\r\nOK\r\n",
				.rsp_handler = NULL,
				.data = NULL
			}
		},
		.err = "\r\n+CME ERROR: ",
		.comm_timeout = 100
	}
};

static bool wait_for_clean_restart(uint32_t timeout_ms)
{
	/*
	 * Sending at command right after reset command succeeds which is not
	 * desirable, wait here for few seconds before we send at command to
	 * poll for modem
	 */
	sys_delay(3000);

	/* Make sure the modem is ready to accept commands */
	uint32_t start = sys_get_tick_ms();
	uint32_t end;
	at_ret_code result = AT_FAILURE;
	while (result != AT_SUCCESS) {
		end = sys_get_tick_ms();
		if ((end - start) > MODEM_RESET_DELAY) {
			DEBUG_V0("%s: timed out\n", __func__);
			return false;
		}
		result = at_core_wcmd(&modem_core[MODEM_OK], false);
		sys_delay(CHECK_MODEM_DELAY);
	}
	return true;
}

void at_modem_hw_reset(void)
{
	gpio_write(MODEM_HW_RESET_PIN, PIN_LOW);
	sys_delay(RESET_PULSE_WIDTH_MS);
	gpio_write(MODEM_HW_RESET_PIN, PIN_HIGH);

	wait_for_clean_restart(MODEM_RESET_DELAY);
}

bool at_modem_sw_reset(void)
{
	at_ret_code res = at_core_wcmd(&modem_core[MODEM_RESET], false);
	if (res == AT_SUCCESS) {
		if (!wait_for_clean_restart(MODEM_RESET_DELAY))
			return false;
		return true;
	}
	return false;
}

bool at_modem_configure(void)
{
	/* Switch off TX echo */
	at_ret_code result = at_core_wcmd(&modem_core[ECHO_OFF], false);
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

bool at_modem_get_nstat(void)
{
	at_ret_code res = at_core_wcmd(&modem_core[EPS_REG_QUERY], true);
	if (res != AT_SUCCESS)
		return false;

	res = at_core_wcmd(&modem_core[ExPS_REG_QUERY], true);
	if (res != AT_SUCCESS)
		return false;

	return true;
}

static bool process_network_urc(const char *urc, enum at_urc u_code)
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
	if (!gpio_init(MODEM_HW_RESET_PIN, &reset_pins))
		return false;
	gpio_write(MODEM_HW_RESET_PIN, PIN_HIGH);

	return true;
}

static bool get_param(enum modem_query_commands cmd, char *buf)
{
	bool ret = false;
	if (buf == NULL)
		goto exit_func;

	if (!at_tcp_enter_cmd_mode())
		goto exit_func;

	memset(buf, 0, buf_len[cmd]);
	modem_query[cmd].rsp_desc[0].data = buf;

	if (at_core_wcmd(&modem_query[cmd], true) != AT_SUCCESS)
		goto exit_func;

	if (buf[0] == 0x00)
		goto exit_func;

	ret = true;
exit_func:
	if (!at_tcp_leave_cmd_mode())
		ret = false;
	
	return ret;
}

bool at_modem_get_ip(char *ip)
{
	return get_param(GET_IP_ADDR, ip);
}

bool at_modem_get_imei(char *imei)
{
	return get_param(GET_IMEI, imei);
}

bool at_modem_get_ss(char *ss)
{
	return get_param(GET_SIG_STR, ss);
}

bool at_modem_get_iccid(char *iccid)
{
	return get_param(GET_ICCID, iccid);
}

bool at_modem_get_ttz(char *ttz)
{
	return get_param(GET_TTZ, ttz);
}

bool at_modem_get_man_info(char *man)
{
	return get_param(GET_MAN_INFO, man);
}

bool at_modem_get_mod_info(char *mod)
{
	return get_param(GET_MOD_INFO, mod);
}

bool at_modem_get_fwver(char *fwver)
{
	return get_param(GET_FWVER, fwver);
}

bool at_modem_get_imsi(char *imsi)
{
	return get_param(GET_IMSI, imsi);
}
