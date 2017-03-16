/**
 * \file platform.h
 *
 * \brief APIs to address platform related init and other utillities
 *
 * \copyright Copyright (C) 2016, Verizon. All rights reserved.
 *
 *
 */
#ifndef PLAT_H
#define PLAT_H

#include <stdint.h>

 /**
  * \brief       Initializes platform, includes HAL layer init, system clock
  *              configuration, installing different handlert etc...
  */
void platform_init();

/**
 * \brief       Provides delay functionality
 *
 * \param[in] delay_ms    delay value in miliseconds
 */
void platform_delay(uint32_t delay_ms);

/**
 * \brief       Provides system tick functionality
 *
 * \return      Number of milliseconds since calling platform_init()
 */
uint32_t platform_get_tick_ms(void);

/**
 * \brief	Reset the modem through hardware means
 * \note	This will abruptly reset the modem without saving the current
 * parameters to NVM and without properly detaching from the network.
 */
void platform_reset_modem(void);

#endif
