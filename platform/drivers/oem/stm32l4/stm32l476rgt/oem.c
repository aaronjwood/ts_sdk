/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "utils.h"
#include "oem_hal.h"
#include "oem_defs.h"
#include "dbg.h"
#include "cJSON.h"

static void init_device_profile(void);
static void init_ipaddr_profile(void);
static void init_network_profile(void);

static oem_profile_t oem_prof_data[NUM_PROF] = {
	[DEVINFO_INDEX] = {
		"DINF",
		g_chrt_device_info,
		sizeof(g_chrt_device_info) / sizeof(oem_char_t),
		init_device_profile
	},
	[RAM_INDEX] = {
		"RAM",
		g_chrt_ram,
		sizeof(g_chrt_ram) / sizeof(oem_char_t),
		NULL
	},
	[NW_INDEX] = {
		"NW",
		g_chrt_network,
		sizeof(g_chrt_network) / sizeof(oem_char_t),
		init_network_profile
	},
	[STRG_INDEX] = {
		"STRG",
		g_chrt_storage,
		sizeof(g_chrt_storage) / sizeof(oem_char_t),
		NULL
	},
	[IPADDR_INDEX] = {
		"IPADDR",
		g_chrt_ipaddr,
		sizeof(g_chrt_ipaddr) / sizeof(oem_char_t),
		init_ipaddr_profile
	},
};

static uint16_t num_profiles = (sizeof(oem_prof_data) /
					sizeof(oem_profile_t));
static oem_hash_table_t hash_table[HASH_BUCKET_SZ][HASH_CHAIN_SZ];
static oem_hash_table_t hash_table_char[CHAR_BUCKET_SZ][CHAR_CHAIN_SZ];

/* The function was adapted from Compilers: Principles, Techniques, and
* Tools (Reading, MA: Addison-Wesley,1986)
*/
static int hash_function(const char *key, int buck_sz)
{
	if (!key)
		return INVALID;
	const char *ptr = key;
	unsigned int val = 0;
	while (*ptr != '\0') {
		unsigned int tmp;
		val = (val << 4) + (*ptr);
		tmp = val & 0xf0000000;
		if (tmp) {
			val = val ^ (tmp >> 24);
			val = val ^ tmp;
		}
		ptr++;
	}
	return (val % buck_sz);

}

static void insert_to_hash_table(int key, int gid, int cid)
{
	if (gid == INVALID)
		return;
	if (key == INVALID)
		return;
	int index = INVALID;
	int chain_sz;
	if (cid == INVALID) {
		for (int i = 0; i < HASH_CHAIN_SZ; i++) {
			if (hash_table[key][i].oem_chr_name == NULL) {
				index = i;
				break;
			}
		}
		chain_sz = HASH_CHAIN_SZ;
	} else {
		for (int i = 0; i < CHAR_CHAIN_SZ; i++) {
			if (hash_table_char[key][i].oem_chr_name == NULL) {
				index = i;
				break;
			}
		}
		chain_sz = CHAR_CHAIN_SZ;
	}

	const char *char_name = NULL;
	const char *prof_name = NULL;
	if (cid != INVALID)
		char_name = oem_prof_data[gid].oem_char[cid].chr_short_name;
	prof_name = oem_prof_data[gid].grp_short_name;
	if ((index < chain_sz) && (index != INVALID)) {
		if (cid != INVALID) {
			hash_table_char[key][index].oem_chr_name = char_name;
			hash_table_char[key][index].grp_indx = gid;
			hash_table_char[key][index].chr_indx = cid;
		} else {
			hash_table[key][index].oem_chr_name = prof_name;
			hash_table[key][index].grp_indx = gid;
		}
	} else
		dbg_printf("%s:%d: Hash table congestion, "
			"increase the HASH_CHAIN_SZ\n", __func__, __LINE__);
}

static void create_hash_table_for_profiles(void)
{
	int key = 0;
	/* loop to store the OEM group profiles into hash table */
	for (uint16_t i = 0; i < num_profiles; i++) {
		key = hash_function(oem_prof_data[i].grp_short_name,
			HASH_BUCKET_SZ);
		/*Oem Group will not have characterisitic index */
		insert_to_hash_table(key, i, INVALID);
	}
}

static void create_hash_table_for_char(void)
{
	int key = 0;
	/* Loop to store the OEM characteristics into hash table */
	for (int i = 0; i < num_profiles; i++) {
		for (int j = 0; j < oem_prof_data[i].chr_count; j++) {
			key = hash_function(
				oem_prof_data[i].oem_char[j].chr_short_name,
				CHAR_BUCKET_SZ);
			insert_to_hash_table(key, i, j);
		}
	}
}

static void init_device_profile(void)
{
	int val_size;
	char na[] = "N/A";
	bool failed = false;

	for (int i = 0; i < oem_prof_data[DEVINFO_INDEX].chr_count; i++) {
		const char *char_name =
			oem_prof_data[DEVINFO_INDEX].oem_char[i].chr_short_name;
		char *cur_value =
				oem_prof_data[DEVINFO_INDEX].oem_char[i].value;
		val_size =
			sizeof(oem_prof_data[DEVINFO_INDEX].oem_char[i].value);
		memset(cur_value, 0, val_size);

		if (!(strcmp(char_name, "IMEI")) ||
		!(strcmp(char_name, "DID"))) {
			if (!utils_get_device_id(cur_value, val_size, 0))
				failed = true;
		} else if (!strcmp(char_name, "IMSI")) {
			if (!utils_get_imsi(cur_value, val_size))
				failed = true;
		} else if (!strcmp(char_name, "MNF")) {
			if (!utils_get_manufacturer(cur_value, val_size))
				failed = true;
		} else if (!strcmp(char_name, "MOD")) {
			if (!utils_get_dev_model(cur_value, val_size))
				failed = true;
		} else if (!strcmp(char_name, "ICCID")) {
			if (!utils_get_iccid(cur_value, val_size))
				failed = true;
		} else if (!strcmp(char_name, "TZ")) {
			if (!utils_get_time_zone(cur_value, val_size))
				failed = true;
		} else if (!strcmp(char_name, "CHIP")) {
			if (!utils_get_chipset(cur_value, val_size))
				failed = true;
		} else if (!strcmp(char_name, "BID")) {
			if (!utils_get_fwbid(cur_value, val_size))
				failed = true;
		} else
			strncpy(cur_value, na, strlen(na));

		if (failed) {
			strncpy(cur_value, na, strlen(na));
			failed = false;
		}
	} /* For loop ends */
}

static void init_network_profile(void)
{
	int value_sz;
	char na[] = "N/A";
	bool failed = false;

	for (int i = 0; i < oem_prof_data[NW_INDEX].chr_count; i++) {
		const char *char_name =
			oem_prof_data[NW_INDEX].oem_char[i].chr_short_name;
		char *cur_value = oem_prof_data[NW_INDEX].oem_char[i].value;
		value_sz = sizeof(oem_prof_data[NW_INDEX].oem_char[i].value);
		memset(cur_value, 0, value_sz);

		if (!(strcmp(char_name, "4gSIS"))) {
			if (!utils_get_sis(cur_value, value_sz))
				failed = true;
		} else
			strncpy(cur_value, na, strlen(na));

		if (failed) {
			strncpy(cur_value, na, strlen(na));
			failed = false;
		}
	} /* For loop ends */
}


static void init_ipaddr_profile(void)
{
	int val_sz = sizeof(oem_prof_data[IPADDR_INDEX].oem_char[IP].value);
	memset(oem_prof_data[IPADDR_INDEX].oem_char[IP].value, 0, val_sz);
	if (!utils_get_ip_addr(oem_prof_data[IPADDR_INDEX].oem_char[IP].value,
		val_sz, NET_INTFC)) {
		strncpy(oem_prof_data[IPADDR_INDEX].oem_char[IP].value, "N/A",
			strlen("N/A"));
	}
}

static void init_hash_tables()
{
	memset(hash_table, 0, sizeof(hash_table));
	memset(hash_table_char, 0, sizeof(hash_table_char));
	for (int i = 0; i < HASH_BUCKET_SZ; i++) {
		for (int j = 0; j < HASH_CHAIN_SZ; j++) {
			hash_table[i][j].chr_indx = INVALID;
			hash_table[i][j].grp_indx = INVALID;
		}
	}

	for (int i = 0; i < CHAR_BUCKET_SZ; i++) {
		for (int j = 0; j < CHAR_CHAIN_SZ; j++) {
			hash_table_char[i][j].chr_indx = INVALID;
			hash_table_char[i][j].grp_indx = INVALID;
		}
	}
}

void oem_init(void)
{
	init_hash_tables();
	init_device_profile();
	init_network_profile();
	init_ipaddr_profile();
	create_hash_table_for_profiles();
	create_hash_table_for_char();
}

uint16_t oem_get_num_of_profiles(void)
{
	return num_profiles;
}

static int get_profile_idx(const char *profile)
{
	int hash_val = hash_function(profile, HASH_BUCKET_SZ);
	if (hash_val < 0)
		return INVALID;
	int index = INVALID;
	for (int i = 0; i < HASH_CHAIN_SZ; i++) {
		if (hash_table[hash_val][i].oem_chr_name != NULL) {
			if (!strcmp(hash_table[hash_val][i].oem_chr_name,
				profile)) {
				index = i;
				break;
			}
		}
	}
	if (index == INVALID)
		return INVALID;
	int idx = hash_table[hash_val][index].grp_indx;

	if ((idx == INVALID) || (idx >= NUM_PROF))
		return INVALID;
	return idx;
}

char *oem_get_profile_info_in_json(const char *profile)
{
	if (!profile)
		return NULL;

	int pro_id = get_profile_idx(profile);
	if (pro_id == INVALID)
		return NULL;
	cJSON *payload = cJSON_CreateObject();
	cJSON *item = cJSON_CreateObject();
	if (!item){
		cJSON_Delete(payload);
		return NULL;
	}
	const char *prof_name = NULL;
	const char *char_name = NULL;
	prof_name = oem_prof_data[pro_id].grp_short_name;
	cJSON_AddItemToObject(payload, prof_name, item);
	for (int i = 0; i < oem_prof_data[pro_id].chr_count; i++) {
		char_name = oem_prof_data[pro_id].oem_char[i].chr_short_name;
		cJSON_AddItemToObject(item, char_name,
		cJSON_CreateString(oem_prof_data[pro_id].oem_char[i].value));
	}
	char *msg = cJSON_PrintUnformatted(payload);
	cJSON_Delete(payload);
	return msg;
}

char *oem_get_all_profile_info_in_json(void)
{

	cJSON *payload = cJSON_CreateObject();
	if (!payload)
		return NULL;
	const char *prof_name = NULL;
	const char *char_name = NULL;
	for (int i = 0; i < num_profiles; i++) {
		cJSON *item = cJSON_CreateObject();
		if (!item) {
			cJSON_Delete(payload);
			return NULL;
		}
		prof_name = oem_prof_data[i].grp_short_name;
		cJSON_AddItemToObject(payload, prof_name, item);

		for (int j = 0; j < oem_prof_data[i].chr_count; j++) {
			char_name = oem_prof_data[i].oem_char[j].chr_short_name;
			cJSON_AddItemToObject(item, char_name,
			cJSON_CreateString(oem_prof_data[i].oem_char[j].value));
		}
	}
	char *msg = cJSON_PrintUnformatted(payload);
	cJSON_Delete(payload);
	return msg;
}

char *oem_get_characteristic_info_in_json(const char *charstc)
{
	if (!charstc)
		return NULL;

	int hash_val = hash_function(charstc, CHAR_BUCKET_SZ);
	if (hash_val < 0)
		return NULL;
	int index = INVALID;
	for (int i = 0; i < CHAR_CHAIN_SZ; i++) {
		if (hash_table_char[hash_val][i].oem_chr_name != NULL) {
			if (!strcmp(hash_table_char[hash_val][i].oem_chr_name,
				charstc)) {
				index = i;
				break;
			}
		}
	}
	if (index == INVALID)
		return NULL;
	int pro_id = hash_table_char[hash_val][index].grp_indx;
	int char_id = hash_table_char[hash_val][index].chr_indx;
	if (pro_id >= NUM_PROF)
		return NULL;

	cJSON *payload = cJSON_CreateObject();
	const char *char_name = NULL;

	char_name = oem_prof_data[pro_id].oem_char[char_id].chr_short_name;
	cJSON_AddItemToObject(payload, char_name,
	cJSON_CreateString(oem_prof_data[pro_id].oem_char[char_id].value));

	char *msg = cJSON_PrintUnformatted(payload);
	cJSON_Delete(payload);
	return msg;
}

void oem_update_profiles_info(const char *profile)
{
	if (!profile) {
		for (int i = 0; i < num_profiles; i++) {
			if (oem_prof_data[i].update_prof)
				oem_prof_data[i].update_prof();
		}
		return;
	}
	int pro_id = get_profile_idx(profile);
	if (pro_id == INVALID)
		return;
	if (oem_prof_data[pro_id].update_prof)
			oem_prof_data[pro_id].update_prof();
}
