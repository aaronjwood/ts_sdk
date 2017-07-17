
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include "utils.h"
#include "oem_hal.h"
#include "oem_defs.h"
#include "dbg.h"
#include "cJSON.h"

static uint16_t num_profiles = (sizeof(oem_prof_data) /
                                        sizeof(oem_profile_t));
static oem_hash_table_t hash_table[HASH_BUCKET_SIZE][HASH_CHAIN_SIZE] = {0};

/* The function was adapted from Compilers: Principles, Techniques, and
 * Tools (Reading, MA: Addison-Wesley,1986)
 */
static int hash_function(const char *key)
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
        printf("Calculated has value: %d\n", val);
        return (val % HASH_BUCKET_SIZE);

}

static void insert_to_hash_table(int key, int gid, int cid, bool acronym_flag)
{
        if ((gid == INVALID) && (key == INVALID))
                return;
        for (int i = 0; i < OEM_HASH_CHAIN_SIZE; i++) {
                if (hash_table[key][i].oem_chr_name == NULL)
                        break;
        }
        char *char_name = NULL;
        char *prof_name = NULL;
        if (cid != INVALID) {
                if (acronym_flag)
                        char_name =
                                oem_prof_data[gid].oem_char[cid].chr_short_name;
                else
                        char_name =
                                oem_prof_data[gid].oem_char[cid].chr_full_name;
        }
        if (acronym_flag)
                prof_name = oem_prof_data[gid].chr_short_name;
        else
                prof_name = oem_prof_data[gid].chr_full_name;

        if (i < OEM_HASH_CHAIN_SIZE) {
                if (cid != INVALID)
                        hash_table[key][i].oem_chr_name = char_name;
                else
                        hash_table[key][i].oem_chr_name = prof_name;
                hash_table[key][i].grp_indx = gid;
                hash_table[key][i].chr_indx = cid;
        } else
                dbg_printf("%s:%d: Hash table congestion,
                        increase the OEM_HASH_CHAIN_SIZE\n", __func__, __LINE__);

        return;
}

static void create_hash_table_for_profiles(void)
{
        unsigned int key = 0;

        /** loop to store the OEM group profiles into hash table */
        for (uint16_t i = 0; i < num_profiles; i++) {
                key = hash_function(oem_prof_data[i].chr_short_name);
                /*Oem Group will not have characterisitic index */
                insert_to_hash_table(key, i, INVALID, true);

                key = hash_function(oem_prof_data[i].chr_full_name);
                /*Oem Group will not have characterisitic index */
                insert_to_hash_table(key, i, INVALID, false);
        }

        /** Loop to store the OEM characteristics into hash table */

        for (int i = 0; i < no_of_oem_profiles; i++) {
                for (int j = 0; j < oem_prof_data[i].chr_count; j++) {
                        key = hash_function(
                                oem_prof_data[i].oem_char[j].chr_short_name);
                        insert_to_hash_table(key, i, j, true);
                        key = hash_function(
                                oem_prof_data[i].oem_char[j].chr_full_name);
                        insert_to_hash_table(key, i, j, false);
                }
        }
}

static void init_device_profile(void)
{
        int i = 0;
        uint8_t *value_buf = NULL;
        char *start = NULL;
        char *end = NULL;
        int16_t time_zone = 0, kernel_version_len = 0;
        int val_size;
        char na[] = "N/A";
        bool failed = false;

        for(i = 0; i < oem_prof_data[DEVINFO_INDEX].chr_count; i++) {
                char *char_name =
                        oem_prof_data[DEVINFO_INDEX].oem_char[i].chr_full_name;
                char *cur_value = oem_prof_data[DEVINFO_INDEX].oem_char[i].value;
                val_size = sizeof(cur_value);
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
        if (!utils_get_ram_info(&free_ram, &avail_ram))
                return;

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
        if !(utils_get_ip_addr(oem_prof_data[IPADDR_INDEX].oem_char[IP].value,
                val_sz, NET_INTFC)) {
                strncpy(oem_prof_data[IPADDR_INDEX].oem_char[IP].value, "N/A",
                        strlen("N/A"));
        }
}

void oem_init(void)
{
        init_device_profile();
        init_ram_profile();
        init_ipaddr_profile();
        create_hash_table_for_profiles();
}

uint16_t oem_get_num_of_profiles(void)
{
        return num_profiles;
}

char *oem_get_profile_info_in_json(const char *profile, bool acronym)
{
        if (!profile)
                return NULL;
        int hash_val = hash_function(profile);
        if (hash_val < 0)
                return NULL;
        int index = INVALID;
        for (int i = 0; i < HASH_CHAIN_SIZE; i++) {
                if (hash_table[hash_val][i].oem_chr_name != NULL) {
                        if (!strcmp(oem_hash_table[hash_val][i].oem_chr_name,
                                profile)) {
                                index = i;
                                break;
                        }
                }
        }
        if (index == INVALID)
                return NULL;
        int pro_id = oem_hash_table[hash_val][index].grp_indx;
        if (pro_id >= NUM_PROF)
                return NULL;

        cJSON *payload = cJSON_CreateObject();
        cJSON *item = cJSON_CreateObject();
        if (!payload && !item)
                return NULL;

        char *prof_name = NULL;
        char *char_name = NULL;
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

char *oem_get_characteristic_info_in_json(const char *charstc, bool acronym)
{
        if (!charstc)
                return NULL;

        int hash_val = hash_function(charstc);
        if (hash_val < 0)
                return NULL;
        int index = INVALID;
        for (int i = 0; i < HASH_CHAIN_SIZE; i++) {
                if (hash_table[hash_val][i].oem_chr_name != NULL) {
                        if (!strcmp(oem_hash_table[hash_val][i].oem_chr_name,
                                charstc)) {
                                index = i;
                                break;
                        }
                }
        }
        if (index == INVALID)
                return NULL;
        int pro_id = oem_hash_table[hash_val][index].grp_indx;
        int char_id = oem_hash_table[hash_val][index].chr_indx;
        if (pro_id >= NUM_PROF)
                return NULL;

        cJSON *payload = cJSON_CreateObject();
        char *char_name = NULL;

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

void oem_update_profiles_info(int grp_indx)
{
        if (grp_indx < 0) {
                for(int i = 0; i < num_profiles; i++) {
                        if (oem_prof_data[i].update_prof)
                                oem_prof_data[i].update_prof();
                }
        } else
                if (oem_prof_data[grp_indx].update_prof)
                        oem_prof_data[grp_indx].update_prof();
}
