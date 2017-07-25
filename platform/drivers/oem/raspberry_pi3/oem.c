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
static void init_ram_profile(void);
static void init_ipaddr_profile(void);

static oem_profile_t oem_prof_data[NUM_PROF] = {
        [DEVINFO_INDEX] = {
                "DINF",
                "DeviceInfo",
                g_chrt_device_info,
                sizeof(g_chrt_device_info) / sizeof(oem_char_t),
                init_device_profile
        },
        [RAM_INDEX] = {
                "RAM",
                "RAM",
                g_chrt_ram,
                sizeof(g_chrt_ram) / sizeof(oem_char_t),
                init_ram_profile
        },
        [NW_INDEX] = {
                "NW",
                "Network",
                g_chrt_network,
                sizeof(g_chrt_network) / sizeof(oem_char_t),
                NULL
        },
        [STRG_INDEX] = {
                "STRG",
                "Storage",
                g_chrt_storage,
                sizeof(g_chrt_storage) / sizeof(oem_char_t),
                NULL
        },
        [BTR_INDEX] = {
                "BTR",
                "Battery",
                g_chrt_battery,
                sizeof(g_chrt_battery) / sizeof(oem_char_t),
                NULL
        },
        [BT_INDEX] = {
                "BT",
                "Bluetooth",
                g_chrt_bluetooth,
                sizeof(g_chrt_bluetooth) / sizeof(oem_char_t),
                NULL
        },
        [IPADDR_INDEX] = {
                "IPADDR",
                "IP Address",
                g_chrt_ipaddr,
                sizeof(g_chrt_ipaddr) / sizeof(oem_char_t),
                init_ipaddr_profile
        },
        [WIFI_INDEX] = {
                "WIFI",
                "WIFI",
                g_chrt_wifi,
                sizeof(g_chrt_wifi) / sizeof(oem_char_t),
                NULL
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

static void insert_to_hash_table(int key, int gid, int cid, bool acronym_flag)
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
        if (cid != INVALID) {
                if (acronym_flag)
                        char_name =
                                oem_prof_data[gid].oem_char[cid].chr_short_name;
                else
                        char_name =
                                oem_prof_data[gid].oem_char[cid].chr_full_name;
        }
        if (acronym_flag)
                prof_name = oem_prof_data[gid].grp_short_name;
        else
                prof_name = oem_prof_data[gid].grp_full_name;

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

        return;
}

static void create_hash_table_for_profiles(void)
{
        int key = 0;
        /* loop to store the OEM group profiles into hash table */
        for (uint16_t i = 0; i < num_profiles; i++) {
                key = hash_function(oem_prof_data[i].grp_short_name,
                        HASH_BUCKET_SZ);
                /*Oem Group will not have characterisitic index */
                insert_to_hash_table(key, i, INVALID, true);

                key = hash_function(oem_prof_data[i].grp_full_name,
                        HASH_BUCKET_SZ);
                /*Oem Group will not have characterisitic index */
                insert_to_hash_table(key, i, INVALID, false);
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
                        insert_to_hash_table(key, i, j, true);
                        key = hash_function(
                                oem_prof_data[i].oem_char[j].chr_full_name,
                                CHAR_BUCKET_SZ);
                        insert_to_hash_table(key, i, j, false);
                }
        }
}

static void init_device_profile(void)
{
        int val_size;
        char na[] = "N/A";
        bool failed = false;

        for(int i = 0; i < oem_prof_data[DEVINFO_INDEX].chr_count; i++) {
                const char *char_name =
                        oem_prof_data[DEVINFO_INDEX].oem_char[i].chr_full_name;
                char *cur_value = oem_prof_data[DEVINFO_INDEX].oem_char[i].value;
                val_size = sizeof(oem_prof_data[DEVINFO_INDEX].oem_char[i].value);
                memset(cur_value, 0, val_size);

                if (!(strcmp(char_name, "IMEI")) ||
                !(strcmp(char_name, "Device ID")) ||
                !(strcmp(char_name, "IMSI"))) {
                        if (!utils_get_device_id(cur_value, val_size, NET_INTFC))
                                failed = true;
                } else if (!strcmp(char_name, "OS Version")) {
                        if (!utils_get_os_version(cur_value, val_size))
                                failed = true;
                } else if (!strcmp(char_name, "Manufacturer")) {
                        if (!utils_get_manufacturer(cur_value, val_size))
                                failed = true;
                } else if (!strcmp(char_name, "Model")) {
                        if (!utils_get_dev_model(cur_value, val_size))
                                failed = true;
                } else if (!strcmp(char_name, "ICCID")) {
                        if (!utils_get_iccid(cur_value, val_size))
                                failed = true;
                } else if (!strcmp(char_name, "Time Zone")) {
                        if (!utils_get_time_zone(cur_value, val_size))
                                failed = true;
                } else if (!strcmp(char_name, "Last Power On")) {
                        if (!utils_get_uptime(cur_value, val_size))
                                failed = true;
                } else if (!strcmp(char_name, "Chipset")) {
                        if (!utils_get_chipset(cur_value, val_size))
                                failed = true;
                } else if (!strcmp(char_name, "Kernel Version") ||
                        !strcmp(char_name, "Build ID")) {
                        if (!utils_get_kernel_version(cur_value, val_size))
                                failed = true;
                } else
                        strncpy(cur_value, na, strlen(na));

                if (failed) {
                        strncpy(cur_value, na, strlen(na));
                        failed = false;
                }
        } /* For loop ends */
}

static void init_ram_profile(void)
{

        uint32_t free_ram = 0;
        uint32_t avail_ram = 0;

        utils_get_ram_info(&free_ram, &avail_ram);
        size_t val_sz = sizeof(oem_prof_data[RAM_INDEX].oem_char[AVRAM].value);

        memset(oem_prof_data[RAM_INDEX].oem_char[AVRAM].value, 0, val_sz);
        snprintf(oem_prof_data[RAM_INDEX].oem_char[AVRAM].value, val_sz,
                "%u", avail_ram);

        val_sz = sizeof(oem_prof_data[RAM_INDEX].oem_char[FRRAM].value);
        memset(oem_prof_data[RAM_INDEX].oem_char[FRRAM].value, 0, val_sz);

        snprintf(oem_prof_data[RAM_INDEX].oem_char[FRRAM].value, val_sz,
                "%u", free_ram);

        val_sz = sizeof(oem_prof_data[RAM_INDEX].oem_char[TLRAM].value);

        memset(oem_prof_data[RAM_INDEX].oem_char[TLRAM].value, 0, val_sz);

        snprintf(oem_prof_data[RAM_INDEX].oem_char[TLRAM].value, val_sz,
                "%u", free_ram + avail_ram);
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
        init_ram_profile();
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

char *oem_get_profile_info_in_json(const char *profile, bool acronym)
{
        if (!profile)
                return NULL;

        int pro_id = get_profile_idx(profile);
        if (pro_id == INVALID)
                return NULL;
        cJSON *payload = cJSON_CreateObject();
        cJSON *item = cJSON_CreateObject();
        if (!payload || !item)
                return NULL;
        const char *prof_name = NULL;
        const char *char_name = NULL;
        if (acronym)
                prof_name = oem_prof_data[pro_id].grp_short_name;
        else
                prof_name = oem_prof_data[pro_id].grp_full_name;
        cJSON_AddItemToObject(payload, prof_name, item);
        for (int i = 0; i < oem_prof_data[pro_id].chr_count; i++) {
                if (acronym)
                        char_name =
                                oem_prof_data[pro_id].oem_char[i].chr_short_name;
                else
                        char_name =
                                oem_prof_data[pro_id].oem_char[i].chr_full_name;

                cJSON_AddItemToObject(item, char_name,
                        cJSON_CreateString(oem_prof_data[pro_id].oem_char[i].value));
        }
        char *msg = cJSON_PrintUnformatted(payload);
        cJSON_Delete(payload);
        return msg;
}

char *oem_get_all_profile_info_in_json(bool acronym)
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
                if (acronym)
                        prof_name = oem_prof_data[i].grp_short_name;
                else
                        prof_name = oem_prof_data[i].grp_full_name;
                cJSON_AddItemToObject(payload, prof_name, item);

                for (int j = 0; j < oem_prof_data[i].chr_count; j++) {
                        if (acronym)
                                char_name =
                                oem_prof_data[i].oem_char[j].chr_short_name;
                        else
                                char_name =
                                oem_prof_data[i].oem_char[j].chr_full_name;

                        cJSON_AddItemToObject(item, char_name,
                        cJSON_CreateString(oem_prof_data[i].oem_char[j].value));
                }
        }
        char *msg = cJSON_PrintUnformatted(payload);
        cJSON_Delete(payload);
        return msg;
}

char *oem_get_characteristic_info_in_json(const char *charstc, bool acronym)
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

        if (acronym)
                char_name =
                        oem_prof_data[pro_id].oem_char[char_id].chr_short_name;
        else
                char_name = oem_prof_data[pro_id].oem_char[char_id].chr_full_name;

        cJSON_AddItemToObject(payload, char_name,
                cJSON_CreateString(oem_prof_data[pro_id].oem_char[char_id].value));

        char *msg = cJSON_PrintUnformatted(payload);
        cJSON_Delete(payload);
        return msg;
}

void oem_update_profiles_info(const char *profile)
{
        if (!profile) {
                for(int i = 0; i < num_profiles; i++) {
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
