/**
 * \file platform.h
 *
 * \brief APIs to address platform related init and other utillities
 *
 * \copyright Copyright (C) 2016, 2017 Verizon. All rights reserved.
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
 * \brief       Provides slee functionality. It will return on any event i.e.
 *              hardware interrupts or system events.
 *
 * \param[in] sleep    sleep time value in miliseconds
 * \returns
 * 	0 if sleep was completed uninterrupted or remaining sleep time in milli
 *      seconds.
 */
uint32_t platform_sleep_ms(uint32_t sleep_ms);

#endif
