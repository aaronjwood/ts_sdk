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
 * \note
 * This function needs to be called atleast once within 49 days since boot up
 * for stm32f429 platform.
 */
uint64_t platform_get_tick_ms(void);

/**
 * \brief       Provides sleep functionality. It will return on any event i.e.
 *              hardware interrupts or system events.
 *
 * \param[in] sleep_ms    sleep time value in miliseconds
 * \returns
 * 	0 if sleep was completed uninterrupted or remaining sleep time in milli
 *      seconds.
 * \note
 * This function stops systick timer before going into sleep and resumes it
 * right after wake up for stm32f429 platform.
 */
uint32_t platform_sleep_ms(uint32_t sleep_ms);

/**
 * \brief	Reset the modem through hardware means
 * \param[in] pulse_width_ms	Width of the pulse (in ms) needed to reset the modem
 * \note	This will abruptly reset the modem without saving the current
 * parameters to NVM and without properly detaching from the network.
 */
void platform_reset_modem(uint16_t pulse_width_ms);

#endif
