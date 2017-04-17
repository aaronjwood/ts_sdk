/**
 * \file timer_hal.h
 * \copyright Copyright (c) 2017 Verizon. All rights reserved.
 * \brief Defines the interface for a hardware timer.
 * \details The underlying timer peripheral fires at a programmed period causing
 * the user callback to be invoked. It can be started and stopped independently
 * of other timers on the target and can be queried to check if it is running.
 */
#ifndef TIMER_HAL_H
#define TIMER_HAL_H

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief A user defined callback that is invoked whenever the programmed period
 * of time elapses.
 */
typedef void (*timercallback_t)(void);

/**
 * \brief Interface to the timer peripheral.
 */
typedef struct {
	/** Initialize the period */
	bool (*init_period)(uint32_t period);

	/** Register a callback */
	void (*reg_callback)(timercallback_t cb);

	/** Check if the timer is currently running */
	bool (*is_running)(void);

	/** Start the timer */
	void (*start)(void);

	/** Stop the timer */
	void (*stop)(void);

	/** IRQ handler of this instance */
	void (*irq_handler)(void);
} timer_interface_t;

/**
 * \brief Initialize the timer peripheral instance.
 * \details The instance is programmed with a period and a user callback is
 * registered to it. The units of the period depend on the underlying timer
 * peripheral instance. This routine must be called once for every instance
 * before starting and stopping timer peripherals.
 *
 * \param[in] inst Pointer to the interface of the timer instance.
 * \param[in] period Period of the timer.
 * \param[in] callback Pointer to the callback that will be invoked at the end of
 * every period.
 *
 * \retval true Timer instance was successfully configured.
 * \retval false Failed to initialize the timer instance.
 */
bool timer_init(const timer_interface_t * const inst,
		uint32_t period,
		timercallback_t callback);

/**
 * \brief Query to check if the timer is running.
 * \param[in] inst Pointer to the interface of the timer instance.
 * \retval true Timer is currently running.
 * \retval false Timer is currently disabled.
 */
bool timer_is_running(const timer_interface_t * const inst);

/**
 * \brief Start the timer instance.
 * \details Once started, the timer will continue invoking the user defined
 * callback at the end of each period.
 * \param[in] inst Pointer to the interface of the timer instance.
 * \pre \ref timer_init must be called before using this routine.
 */
void timer_start(const timer_interface_t * const inst);

/**
 * \brief Stop the timer instance.
 * \param[in] inst Pointer to the interface of the timer instance.
 * \pre \ref timer_init must be called before using this routine.
 */
void timer_stop(const timer_interface_t * const inst);

/**
 * \brief Call the IRQ handler of the timer peripheral instance.
 * \details In most cases each peripheral has its own interrupt vector. For
 * this scenario, the handler is invoked from within the driver implicitly and
 * the user need not bother with this routine.
 * However sometimes two peripherals may share the interrupt vector. In such
 * cases, this function can be used to explicitly call the IRQ handler from
 * outside the driver.
 *
 * \param[in] inst Pointer to the interface of the timer instance.
 */
void timer_irq_handler(const timer_interface_t * const inst);

#endif
