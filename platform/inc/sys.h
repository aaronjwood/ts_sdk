/**
 * \file sys.h
 *
 * \brief APIs to address platform related init and other utillities
 *
 * \copyright Copyright (C) 2016, 2017 Verizon. All rights reserved.
 *
 *
 */
#ifndef SYS_H
#define SYS_H

#include <stdint.h>

 /**
  * \brief       Initializes platform, includes HAL layer init, system clock
  *              configuration, installing different handlert etc...
  */
void sys_init(void);

/**
 * \brief       Provides delay functionality
 *
 * \param[in] delay_ms    delay value in miliseconds
 */
void sys_delay(uint32_t delay_ms);

/**
 * \brief       Provides system tick functionality
 *
 * \return      Number of milliseconds since calling sys_init()
 * \note
 * This function needs to be called at least once within 49 days since boot up
 * for all platforms with a 32-bit clock register.
 */
uint64_t sys_get_tick_ms(void);

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
 * \brief	data synchronous barrier
 */
void dsb(void);

#endif
