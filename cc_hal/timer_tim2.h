/**
 * \file timer_tim2.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Defines the interface for the TIM2 timer peripheral.
 * \details This timer peripheral is configured to have a resolution of 1 micro-
 * second and a counter width of 32-bits.
 */
#ifndef TIMER_TIM2_INTERFACE_H
#define TIMER_TIM2_INTERFACE_H

#include "timer_hal.h"

/**
 * \brief Retrieve the interface of the TIM2 timer.
 * \returns Pointer to TIM2's interface.
 */
const timer_interface_t *timer_tim2_get_interface(void);

#endif
