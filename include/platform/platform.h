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
#include <stdbool.h>

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
 * \brief	Raise / lower the interrupt priority of the system tick
 *
 * \param[in] raise_priority	Set to true to raise priority, false to lower it.
 */
void platform_raise_tick_priority(bool raise_priority);

#endif
