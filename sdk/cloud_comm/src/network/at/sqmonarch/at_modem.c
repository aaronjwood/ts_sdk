/* Copyright (C) 2017 Verizon. All rights reserved. */

#include <string.h>
#include <stdio.h>
#include "at_core.h"
#include "at_modem.h"
#include "sys.h"
#include "gpio_hal.h"
#include "ts_sdk_modem_config.h"
#include "at_tcp.h"

#define PWR_EN_DELAY_MS			1000
#define RESET_N_DELAY_MS		1000

#define STABLE_TIMEOUT_MS		5000	/* Wait until modem fully restarts */

enum modem_core_commands {
	MODEM_OK,	/* Simple modem check command, i.e. AT */
	CME_CONF,	/* Change the error output format to numerical */
	MODEM_RESET,	/* Command to reset the modem */
	SIM_READY,	/* Check if the SIM is present */
	EPS_URC_SET,	/* Set the EPS registration URC */
	EPS_REG_QUERY,	/* Query EPS registration */
	MODEM_CORE_END	/* End-of-commands marker */
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

/* Minimum lengths of the buffers including the terminating NULL character */
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

static bool sys_res_stable;		/* True when system is stable after restart*/

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
	while (rcv_bytes[i] != '"') {
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
		.comm_timeout = 100
	},
	[GET_IP_ADDR] = {
		.comm = "at+cgpaddr="MODEM_PDP_CTX"\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+CGPADDR: "MODEM_PDP_CTX"",
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
		.comm_timeout = 5000
	},
	[GET_ICCID] = {
		.comm = "at+sqnccid\r",
		.rsp_desc = {
			{
				.rsp = "\r\n+SQNCCID: \"",
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
		.comm_timeout = 5000
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
		.comm_timeout = 5000
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
		.comm_timeout = 5000
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
		.comm_timeout = 5000
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
		.comm_timeout = 5000
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
		.comm_timeout = 5000
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

bool at_modem_get_nstat(void)
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
