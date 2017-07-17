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
