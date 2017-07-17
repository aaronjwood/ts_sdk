/**
 * \file sys.h
 *
 * \brief APIs to get/set OEM related parameters
 *
 * \copyright Copyright (C) 2017 Verizon. All rights reserved.
 *
 *
 */

#ifndef __OEM_HAL
#define __OEM_HAL


/** Hash functions */
void ts_oem_create_hash_table_for_oem_profiles(void);
int ts_oem_hash_function(const char *str);
void ts_oem_insert_node_to_hash_table(int key, int16_t grp_indx, int16_t chr_indx, bool acronym_flag);

/**
 * \file oem_hal.h
 *
 * \brief APIs to retrieve oem related parameters.
 *
 * \copyright Copyright (C) 2017 Verizon. All rights reserved.
 *
 *
 */
#ifndef OEM_H
#define OEM_H

#include <stdint.h>

 /**
  * \brief       Initializes oem profiles.
  */
void oem_init(void);


/**
 * \brief       provides total number of profiles that are present.
 *
 * \return      Number of profiles or 0 otherwise.
 *
 */
uint16_t oem_get_num_of_profiles(void);

/**
 * \brief       Retreives profile and its characterisitics in json format.
 *
 * \param[in] profile    Valid null terminated profile string name.
 * \param[in] acronym    If true use short form name of the profiles and its
                         characterisitics, full names otherwise.
 * \returns              JSON formatted message or NULL if fails.
 */
char *oem_get_profile_info_in_json(const char *profile, bool acronym);

/**
 * \brief       Retreives specific characterisitic in json format.
 *
 * \param[in] charstc    Valid null terminated characterisitic string name.
 * \param[in] acronym    If true use short form name of characterisitic,
                         full names otherwise.
 * \returns              JSON formatted message or NULL if fails.
 */
char *oem_get_characteristic_info_in_json(const char *charstc, bool acronym);

/**
 * \brief       updates profile with latest values.
 *
 * \param[in] profile    Valid null terminated profile string name.
 */
void oem_update_profiles_info(const char *profile);
#endif

#endif
