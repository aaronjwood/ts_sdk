
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include "utils.h"
#include "oem_hal.h"
#include "oem_defs.h"

static oem_char_t g_chrt_device_info[DEV_PROF_END] = {
        [BBV] = {
                "BBV",
                "Baseband Version",
                "N/A"
        },
        [BID] = {
                "BID",
                "Build ID",
                "N/A"
        },
        [DID] = {
                "DID",
                "Device ID",
                "N/A"
        },
        [ICCID] = {
                "ICCID",
                "ICCID",
                "N/A"
        },
        [IMEI] = {
                "IMEI",
                "IMEI",
                "N/A"
        },
        [IMSI] = {
                "IMSI",
                "IMSI",
                "N/A"
        },
        [KEV] = {
                "KEV",
                "Kernel Version",
                "N/A"
        },
        [LNG] = {
                "LNG",
                "Language",
                "N/A"
        },
        [LPO] = {
                "LPO",
                "Last Power On",
                "N/A"
        },
        [MNF] = {
                "MNF",
                "Manufacturer",
                "N/A"
        },
        [MOD] = {
                "MOD",
                "Model",
                "N/A"
        },
        [CHIP] = {
                "CHIP",
                "Chipset",
                "N/A"
        },
        [NFC] = {
                "NFC",
                "NFC",
                "N/A"
        },
        [NLP] = {
                "NLP",
                "Network Location Provider",
                "N/A"
        },
        [OSV] = {
                "OSV",
                "OS Version",
                "N/A"
        },
        [RTD] = {
                "RTD",
                "Rooted",
                "N/A"
        },
        [SCRS] = {
                "SCRS",
                "Screen Size",
                "N/A"
        },
        [SCRT] = {
                "SCRT",
                "Screen Timeout",
                "N/A"
        },
        [SGPS] = {
                "SGPS",
                "Standalone GPS",
                "N/A"
        },
        [TZ] = {
                "TZ",
                "Time Zone",
                "N/A"
        },
};

static oem_char_t g_chrt_ipaddr[IP_PROF_END] = {
        [IP] = {
                "RU0I41",
                "rmnet_usb0_IPV4_1",
                "N/A"
        },
};

static oem_char_t g_chrt_ram[RAM_PROF_END] = {
        [AVRAM] = {
                "AVRAM",
                "RAM Available",
                "N/A"
        },
        [FRRAM] = {
                "FRRAM",
                "RAM Free",
                "N/A"
        },
        [TLRAM] = {
                "TLRAM",
                "RAM Total",
                "N/A"
        },
};

static oem_profile_t oem_prof_data[NUM_PROF] = {
        [DEVINFO_INDEX] = {
                "DINF",
                "DeviceInfo",
                g_chrt_device_info,
                sizeof(g_chrt_device_info) / sizeof(oem_char_t),
                NULL
        },
        [IPADDR_INDEX] = {
                "IPADDR",
                "IP Address",
                g_chrt_OEM_Ipaddr,
                sizeof(g_chrt_ipaddr) / sizeof(oem_char_t),
                ts_oem_init_ipaddr_profile
        },
        [RAM_INDEX] = {
                "RAM",
                "RAM",
                g_chrt_ram,
                sizeof(g_chrt_ram) / sizeof(oem_char_t),
                NULL
        },

};

static int no_of_oem_profiles = (sizeof(oem_prof_data) / sizeof(oem_profile_t));

static oem_hash_table_t oem_hash_table[HASH_BUCKET_SIZE][HASH_CHAIN_SIZE] = {0};


/**
 * @func int ts_sdk_oem_hash_function(const char *str)
 * @brief API will generate the hash key for the given string
 * @param str pointer points to string
 * @return hash_key if success else EFAILURE
 */
int ts_oem_hash_function(const char *str)
{
  unsigned int long long hash_key = 0;
  int i = 0;
  int hash_const = 31;
  if(NULL == str)
    return EFAILURE;

  for(i = 0; str[i] != '\0'; i++)
  {
    hash_key = hash_key * hash_const + str[i];
  }
  return (hash_key % OEM_HASH_BUCKET_SIZE);

}

/**
 * @Func ts_oem_insert_node_to_hash_table
 * @brief API will insert the node into hash table
 * @param key - index to insert the node
 * @param grp_indx - group index of the oem profile
 * @param chr_indx - characteristic index of the oem profile
 * @param acronym_flag - indicates wheather it is acronym for complete name
 * @return
 */
void ts_oem_insert_node_to_hash_table(int key, int16_t grp_indx, int16_t chr_indx, bool acronym_flag)
{
  int i = 0;
  if((grp_indx == EFAILURE) && (key == EFAILURE))
    return;
  for(i = 0; i < OEM_HASH_CHAIN_SIZE; i++) {
    if(oem_hash_table[key][i].oem_chr_name == NULL)
      break;
  }
  if(i < OEM_HASH_CHAIN_SIZE) {
    if(chr_indx != EFAILURE) {
      if(acronym_flag)
        oem_hash_table[key][i].oem_chr_name = oem_prof_data[grp_indx].oem_char[chr_indx].oem_chr_acronym_name;
      else
        oem_hash_table[key][i].oem_chr_name =
            oem_prof_data[grp_indx].oem_char[chr_indx].chr_full_name;
    } else {
      if(acronym_flag)
        oem_hash_table[key][i].oem_chr_name = oem_prof_data[grp_indx].oem_grp_acronym_name;
      else
        oem_hash_table[key][i].oem_chr_name = oem_prof_data[grp_indx].oem_group_complete_name;
    }

    oem_hash_table[key][i].grp_indx = grp_indx;
    oem_hash_table[key][i].chr_indx = chr_indx;
  }
  else
    TS_LOG_DEBUG("Hash table congestion, need to increase the *OEM_HASH_CHAIN_SIZE* \n");

  return;
}
/**
 * @Func void ts_oem_create_hash_table_for_oem_profiles()
 * @brief API will create the hash table for all OEM Profiles
 * @param void
 * @return void
 */
void ts_oem_create_hash_table_for_oem_profiles()
{
  int i, j;
  int key = 0;
  for(i = 0; i < OEM_HASH_BUCKET_SIZE; i++)
  {
    for(j = 0; j < OEM_HASH_CHAIN_SIZE; j++)
      oem_hash_table[i][j].oem_chr_name = NULL;
  }

  /** loop to store the OEM group profiles into hash table */
  for(i = 0; i < no_of_oem_profiles; i++) {
    key = ts_oem_hash_function(oem_prof_data[i].oem_grp_acronym_name);
    /*Oem Group will not have characterisitic index */
    ts_oem_insert_node_to_hash_table(key, i, EFAILURE, true);

    key = ts_oem_hash_function(oem_prof_data[i].oem_group_complete_name);
    /*Oem Group will not have characterisitic index */
    ts_oem_insert_node_to_hash_table(key, i, EFAILURE, false);
  }

  /** Loop to store the OEM characteristics into hash table */

  for(i = 0; i <= IPADDR_PROFILE_INDEX; i++) {
    for(j = 0; j < oem_prof_data[i].chr_count; j++) {
      key = ts_oem_hash_function(oem_prof_data[i].oem_char[j].oem_chr_acronym_name);
      ts_oem_insert_node_to_hash_table(key, i, j, true);
      key = ts_oem_hash_function(oem_prof_data[i].oem_char[j].chr_full_name);
      ts_oem_insert_node_to_hash_table(key, i, j, false);
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

static void init_ipaddr_profile()
{
        int val_sz = sizeof(oem_prof_data[IPADDR_INDEX].oem_char[IP].value);
        memset(oem_prof_data[IPADDR_INDEX].oem_char[IP].value, 0, val_sz);
        if !(utils_get_ip_addr(oem_prof_data[IPADDR_INDEX].oem_char[IP].value,
                val_sz, NET_INTFC)) {
                strncpy(oem_prof_data[IPADDR_INDEX].oem_char[IP].value, "N/A",
                        strlen("N/A"));
        }
}

void ts_oem_update_profiles_info(int16_t grp_indx)
{
  int i = 0;
  if(EFAILURE == grp_indx)
  {
    for(i = 0; i < no_of_oem_profiles; i++)
    {
      if(NULL != oem_prof_data[i].update_prof)
        oem_prof_data[i].update_prof();
    }
  } else {
    if(NULL != oem_prof_data[grp_indx].update_prof)
      oem_prof_data[grp_indx].update_prof();
  }
}

void oem_init(void)
{
        init_device_profile();
        init_ram_profile();
        init_ipaddr_profile();
}
