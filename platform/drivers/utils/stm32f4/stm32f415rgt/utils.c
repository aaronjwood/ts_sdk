 /*Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "dbg.h"
#include "at_modem.h"

bool utils_get_ip_addr(char *ipaddr, uint8_t length, const char *interface)
{
	if (!ipaddr || length == 0) {
		dbg_printf("%s:%d: invalid parameters\n", __func__, __LINE__);
		return false;
	}

	char ip[16] = {0};
	if (at_modem_get_ip(ip)) {
		snprintf(ipaddr, length, "%s", ip);	
		return true;
	} else {
		 dbg_printf("F415: %s:%d: at_modem_get_ip is failed\n",
				__func__, __LINE__);
		return false;
	}
}

bool utils_get_device_id(char *id, uint8_t len, char *interface)
{
	if (!id || (len == 0))
                return false;

	char d_id[21];
	int i=0;
	
	if ((at_modem_get_imei(d_id))) {
		/* Removing leading zeros from imei to support thingspace portal */
		while(d_id[i] == '0')
			i++;
		snprintf(id, len, "%s", &d_id[i]);
		return true;
	} else {
		dbg_printf("F415: %s:%d: at_modem_get_imei is failed\n",
					__func__, __LINE__);
		return false;
	}
}

bool utils_get_manufacturer(char *value, uint32_t len)
{
	if (!value || (len == 0))
		return false;
	char minfo[16];

	if ((at_modem_get_man_info(minfo))) {
		snprintf(value, len, "%s", minfo);
		return true;
	} else {
		 dbg_printf("F415: %s:%d: at_modem_get_man_info is failed\n",
				__func__, __LINE__);
		return false;
	}
}


bool utils_get_dev_model(char *value, uint32_t len)
{
	if (!value || (len == 0))
		return false;
	char model[16];

	if ((at_modem_get_mod_info(model))) {
		snprintf(value, len, "%s", model);
		return true;
	} else {
		dbg_printf("F415: %s:%d: at_modem_get_mod_info is failed\n",
					__func__, __LINE__);
		return false;
	}
}

bool utils_get_chipset(char *value, uint32_t len)
{
	if (!value || (len == 0))
		return false;
	snprintf(value, len, "%s", "STM32F415RGT");
	return true;
}

bool utils_get_iccid(char *value, uint32_t len)
{
	if (!value || (len == 0))
		return false;
	char iccid[23];
	if ((at_modem_get_iccid(iccid))) {
		snprintf(value, len, "%s", iccid);
		return true;
	} else {
		 dbg_printf("F415: %s:%d: at_modem_get_iccid is failed\n",
					__func__, __LINE__);
		return false;
	}
}

bool utils_get_local_time(char *value, uint32_t len)
{
	if (!value || (len == 0))
		return false;
	char time[21];

	if ((at_modem_get_ttz(time))) {
		snprintf(value, len, "%s", time);
		return true;
	} else {
		dbg_printf("F415: %s:%d: at_modem_get_ttz is failed\n",
				__func__, __LINE__);
		return false;
	}
}


bool utils_get_time_zone(char *value, uint32_t len)
{
	if (!value || (len == 0))
		return false;

	char tzone[21], *tval;
	if ((at_modem_get_ttz(tzone))) {
		tval = &tzone[17];
		int t = atoi(tval)/4 ;
		tval[1] = (t < 10 && t > -10) ? '0' : '1';
		tval[2] = (t < 10 && t > -10)  ? (-t+48) : ((-t%10)+48);
		char str[] = ":00";
		strcat(tval, str);
		snprintf(value, len, "%s", tval);
		return true;
	} else{
		dbg_printf("F415: %s:%d: at_modem_get_ttz is failed\n",
				__func__, __LINE__);
		return false;
	}
}

bool utils_get_sis(char *value, uint32_t len)
{
	if (!value || (len == 0))
		return false;
	char sigstr[11];
	if ((at_modem_get_ss(sigstr))) {
		snprintf(value, len, "%s", sigstr);
		return true;
	} else {
		dbg_printf("F415: %s:%d: at_modem_get_sis is failed\n",
					__func__, __LINE__);
		return false;
	}
}


bool utils_get_imei(char *value, uint32_t len)
{
	if (!value || (len == 0))
		return false;
	char imei[16];

	if (!(at_modem_get_imei(imei))) {
		snprintf(value, len, "%s", imei);
		return true;
	} else {
		dbg_printf("F415: %s:%d: at_modem_get_imei is failed\n",
					__func__, __LINE__);
		return false;
	}
}

bool utils_get_imsi(char *value, uint32_t len)
{
	if (!value || (len == 0))
		return false;
	char imsi[17];

	if ((at_modem_get_imsi(imsi))) {
		snprintf(value, len, "%s", imsi);
		return true;
	} else {
		dbg_printf("F415: %s:%d: at_modem_get_imsi is failed\n",
				__func__, __LINE__);
		return false;
	}
}

bool utils_get_fwbid(char *value, uint32_t len)
{
	if (!value || (len == 0))
		return false;
	char bid[20];

	if ((at_modem_get_fwver(bid))) {
		snprintf(value, len, "%s", bid);
		return true;
	} else {
		dbg_printf("F415: %s:%d: at_modem_get_fwver is failed\n",
				__func__, __LINE__);
		return false;
	}
}
