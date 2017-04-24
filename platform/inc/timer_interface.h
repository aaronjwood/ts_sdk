/**
 * \file timer_interface.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Defines the generic timer interface timer peripheral.
 * \details Timers defined and implemented here will be used only for single
 * consumer, for example: SDK AT layer uses timer 2 for stm32f429zi nucleo
 * platform which makes it not available to any other modules inside or outside
 * the sdk so to use timers each module or consumer has to implement its timer
 * instance picking timer from \ref timer_id_t enum
 */

#ifndef TIMER_INTERFACE_H
#define TIMER_INTERFACE_H

#include "timer_hal.h"

/**
 * \brief Type that defines timers ids, can be modified this enum if timers
 * for perticular chipset exceeds below limit.
 */
typedef enum timer_id {
	TIMER1,
	TIMER2,
	TIMER3,
	TIMER4,
	TIMER5,
	TIMER6,
	TIMER7,
	TIMER8,
	TIMER9,
	TIMER10,
	TIMER11,
	TIMER12,
	TIMER13,
	TIMER14,
	MAX_TIMERS
} timer_id_t;

/**
 * \brief Retrieve the interface of timer which later can be used for performing
 * various timer hal APIs.
 * \param[in] timer id
 * \returns Pointer to timer tim's interface or NULL if fails or timer instance
 * is not implemented.
 */
const timer_interface_t *timer_get_interface(timer_id_t tim);

#endif
