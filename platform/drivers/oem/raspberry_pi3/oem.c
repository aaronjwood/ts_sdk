
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include "utils.h"
#include "oem_hal.h"

enum device_profile_index {
        BBV,
        BID,
        DID,
        ICCID,
        IMEI,
        IMSI,
        KEV,
        LNG,
        LPO
        MNF,
        MOD,
        CHIP,
        NFC,
        NLP,
        OSV,
        RTD,
        SCRS,
        SCRT,
        SGPS,
        TZ,
        DEV_PROF_END
};

enum ram_profile_index {
        AVRAM,
        FRRAM,
        TLRAM,
        RAM_PROF_END
};

enum ipaddr_profile_index {
        IP,
        IP_PROF_END
};

oem_char_t g_chrt_device_info[DEV_PROF_END] = {
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

oem_char_t g_chrt_ipaddr[IP_PROF_END] = {
        [IP] = {
                "RU0I41",
                "rmnet_usb0_IPV4_1",
                "N/A"
        },
};

oem_char_t g_chrt_ram[RAM_PROF_END] = {
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

oem_profile_t oem_prof_data[NUM_PROF] = {
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

int no_of_oem_profiles = (sizeof(oem_prof_data) / sizeof(oem_profile_t));

oem_hash_table_t oem_hash_table[OEM_HASH_BUCKET_SIZE][OEM_HASH_CHAIN_SIZE] = {0};


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
        oem_hash_table[key][i].oem_chr_name = oem_prof_data[grp_indx].oem_characteristic[chr_indx].oem_chr_acronym_name;
      else
        oem_hash_table[key][i].oem_chr_name =
            oem_prof_data[grp_indx].oem_characteristic[chr_indx].oem_chr_complete_name;
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
      key = ts_oem_hash_function(oem_prof_data[i].oem_characteristic[j].oem_chr_acronym_name);
      ts_oem_insert_node_to_hash_table(key, i, j, true);
      key = ts_oem_hash_function(oem_prof_data[i].oem_characteristic[j].oem_chr_complete_name);
      ts_oem_insert_node_to_hash_table(key, i, j, false);
    }
  }
}

static bool init_device_profile()
{
  int i = 0;
  uint8_t* value_buf = NULL;
//  int32_t chip_set_id = 0;
  char* start = NULL;
  char* end = NULL;
  int16_t time_zone = 0, kernel_version_len = 0;
  int current_buff_size;
#ifdef QAPI_TXM_MODULE
  TXM_MODULE_MANAGER_VERSION_ID
#endif

  current_buff_size = sizeof(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value);
  for(i = 0; i < oem_prof_data[DEVICE_PROFILE_INDEX].chr_count; i++) {
    if(!(strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "IMEI")) ||
        !(strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "Device ID"))) {
      value_buf = ts_param_get_device_serial_number();
      if(NULL != value_buf) {
        memset(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, 0, current_buff_size);
        memscpy(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, current_buff_size, value_buf,
            strlen((char*)value_buf));

      }
    } else if(!strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "IMSI")) {
      value_buf = ts_param_get_imsi_frm_uim();
      if(NULL != value_buf) {
        memset(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, 0, current_buff_size);
        memscpy(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, current_buff_size, value_buf,
            strlen((char*)value_buf));

      }
    } else if(!strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "Build ID")) {
      value_buf = ts_param_get_device_modem_build_id();
      if(NULL != value_buf) {
        memset(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, 0, current_buff_size);
        memscpy(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, current_buff_size, value_buf,
            strlen((char*)value_buf));

      }
    } else if(!strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "OS Version")) {
      value_buf = ts_param_get_device_sw_version();
      if(NULL != value_buf) {
        memset(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, 0, current_buff_size);
        memscpy(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, current_buff_size, value_buf,
            strlen((char*)value_buf));

      }
    } else if(!strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "Manufacturer")) {
      value_buf = ts_param_get_device_manufacturer();
      if(NULL != value_buf) {
        memset(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, 0, current_buff_size);
        memscpy(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, current_buff_size, value_buf,
            strlen((char*)value_buf));

      }
    } else if(!strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "Model")) {
      value_buf = ts_param_get_device_model_number();
      if(NULL != value_buf) {
        memset(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, 0, current_buff_size);
        memscpy(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, current_buff_size, value_buf,
            strlen((char*)value_buf));

      }
    } else if(!strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "ICCID")) {
      value_buf = ts_param_get_iccid_frm_physical_slot();
      if(NULL != value_buf) {
        memset(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, 0, current_buff_size);
        memscpy(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, current_buff_size, value_buf,
            strlen((char*)value_buf));

      }
    } else if(!strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "Time Zone")) {
      time_zone = ts_param_nas_get_network_time();
	  if(time_zone != EFAILURE){
      TS_LOG_INFO("Thingspace : Time zone from NAS :%d hr :%d  min :%d\n", time_zone,time_zone/60,time_zone%60);

	  if(time_zone > 0)
      snprintf(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, current_buff_size, "-%02d:%02d",
          (time_zone / 60), (time_zone < 0? (-1 * (time_zone%60)):(time_zone%60)));
	   else
      snprintf(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, current_buff_size, "%02d:%02d",
          (time_zone / 60), (time_zone < 0? (-1 * (time_zone%60)):(time_zone%60)));
	  }
   }

   else if(!strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "Last Power On" ) ) {
      value_buf = ts_param_get_last_power_on_time();
      if (NULL != value_buf) {
        memset(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, 0, current_buff_size);
        memscpy(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, current_buff_size, value_buf,
            strlen((char*)value_buf));
      }
	  }

    else if(!strcmp(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].oem_chr_complete_name, "Chipset")) {
	  value_buf = ts_param_get_device_chip_id();
      if(NULL != value_buf) {
      strlcpy(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value, (const char *)value_buf,
        sizeof(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[i].current_value));
     }
	}

  }
  memset(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[6].current_value, 0, current_buff_size);
#ifdef QAPI_TXM_MODULE
   start = strstr(_txm_module_manager_version_id, "*");
#else
   start = strstr(_tx_version_id, "*") ;
#endif
    if(start != NULL)
    {
      end = strstr((start + 1), "*");
      if(end != NULL) {
        kernel_version_len = end - start;
        memscpy(oem_prof_data[DEVICE_PROFILE_INDEX].oem_characteristic[6].current_value, current_buff_size, (start + 1),
            (kernel_version_len - 1));
      }
    }
}

static bool init_ram_profile()
{

        uint32_t free_ram = 0;
        uint32_t avail_ram = 0;
        if (!utils_get_ram_info(&free_ram, &avail_ram))
                return false;

        size_t val_sz = sizeof(oem_prof_data[RAM_INDEX].oem_char[0].value);

        memset(oem_prof_data[RAM_INDEX].oem_char[0].value, 0, val_sz);
        snprintf(oem_prof_data[RAM_INDEX].oem_char[0].value, val_sz,
                "%u", avail_ram);

        val_sz = sizeof(oem_prof_data[RAM_INDEX].oem_char[1].value);
        memset(oem_prof_data[RAM_INDEX].oem_char[1].value, 0, val_sz);

        snprintf(oem_prof_data[RAM_INDEX].oem_char[1].value, val_sz,
                "%u", free_ram);

        val_sz = sizeof(oem_prof_data[RAM_INDEX].oem_char[2].value);

        memset(oem_prof_data[RAM_INDEX].oem_char[2].value, 0, val_sz);

        snprintf(oem_prof_data[RAM_INDEX].oem_char[2].value, val_sz,
                "%u", free_ram + avail_ram);
        return true;

}

static bool init_ipaddr_profile()
{
        char ipaddr[IP_BUF_SIZE] = { 0 };
        int ipcount = 0;
        size_t val_sz = 0;

        val_sz = sizeof(oem_prof_data[IPADDR_INDEX].oem_char[0].value);

        if !(utils_get_ip_addr(ipaddr, IP_BUF_SIZE, "eth0"))
                return false;

        char *value = oem_prof_data[IPADDR_INDEX].oem_char[0].value;
        memset(oem_prof_data[IPADDR_INDEX].oem_char[0].value, 0, val_sz);
        memcpy(oem_prof_data[IPADDR_INDEX].oem_char[0].value, val_sz,
                ipaddr, strlen(ipaddr));
        return true;
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

bool oem_init()
{
        /* Initialize device profile characteristics */
        init_device_profile();
        /* Initialize the ram profile characateristics */
        init_ram_profile();
        /* Initialize the IPADDR profile characteristics */
        init_ipaddr_profile();
}
