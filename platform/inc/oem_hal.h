/**
 * \file oem_hal.h
 *
 * \brief APIs to retrieve oem profile related parameters.
 *
 * \copyright Copyright (C) 2017 Verizon. All rights reserved.
 *
 *
 */
#ifndef OEM_H
#define OEM_H

#include <stdint.h>

 /**
  * \brief       Initializes oem profiles with latest value
  * \note
  * This function needs to be called first before any other APIs
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
 * \returns              JSON formatted message or NULL if fails.
 */
char *oem_get_profile_info_in_json(const char *profile);

/**
 * \brief       Retreives profile and its characterisitics in json format.
 *
 * \returns     JSON formatted message or NULL if fails.
 */
char *oem_get_all_profile_info_in_json();

/**
 * \brief       Retreives specific characterisitic in json format.
 *
 * \param[in] charstc    Valid null terminated characterisitic string name.
 * \returns              JSON formatted message or NULL if fails.
 */
char *oem_get_characteristic_info_in_json(const char *charstc);

/**
 * \brief       updates/refreshes profile with latest values. This is useful
 *              especially for the profiles with dynamic characterisitic, like
 *              network signal strength etc...
 *
 * \param[in] profile    Valid null terminated profile string name. If null, it
 *                       will refresh all the profiles available.
 */
void oem_update_profiles_info(const char *profile);

#endif
