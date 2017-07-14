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


#define MAX_MCC_SIZE  3
#define MAX_MNC_SIZE  2

#define OEM_HASH_BUCKET_SIZE  128
#define OEM_HASH_CHAIN_SIZE   5
#define MAX_BUF_SIZE  100
#define IP_BUF_SIZE   18


typedef void (*oem_update_profile)(void);

typedef struct {
        const char *oem_chr_name;
        int16_t  grp_indx;
        int16_t chr_indx;
} oem_hash_table_t;

typedef struct {
        const char *chr_short_name;
        const char *chr_full_name;
        char value[MAX_BUF_SIZE];
} oem_char_t;

typedef struct {
        const char *grp_short_name;
        const char *grp_full_name;
        oem_char_t *oem_char;
        uint32_t chr_count;
        oem_update_profile update_prof;
} oem_profile_t;

enum oem_profiles_index {
        DEVINFO_INDEX,
        IPADDR_INDEX,
        RAM_INDEX,
        NUM_PROF
};

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
  * \brief       Initializes oem profiles
  */
void oem_init(void);


/**
 * \brief       provides total number of profiles that are present.
 *
 * \return      Number of profiles or 0 otherwise.
 *
 */
uint8_t oem_get_total_profile_count(void);

/**
 * \brief       Provides sleep functionality. It will return on any event i.e.
 *              hardware interrupts or system events.
 *
 * \param[in] sleep_ms    sleep time value in miliseconds
 * \returns
 * 	0 if sleep was completed uninterrupted or remaining sleep time in milli
 *      seconds.
 * \note
 * This function stops SysTick timer before going into sleep and resumes it
 * right after wake up for all STM32 MCUs.
 */
uint32_t sys_sleep_ms(uint32_t sleep_ms);

/**
 * \brief	Reset the modem through hardware means
 * \param[in] pulse_width_ms	Width of the pulse (in ms) needed to reset the modem
 * \note	This will abruptly reset the modem without saving the current
 * parameters to NVM and without properly detaching from the network.
 */
void sys_reset_modem(uint16_t pulse_width_ms);

/**
 * \brief	data synchronous barrier
 */
void dsb(void);

#endif

#endif
