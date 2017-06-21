/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdlib.h>
#include <stm32f4xx_hal.h>
#include "timer_interface.h"
#include "timer_hal.h"

typedef struct timer_private {
	TIM_HandleTypeDef *timer_handle;
	timercallback_t cb;
	timer_id_t id;
} timer_private_t;

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))

/* Defines to enable printing of all the errors */
/*#define DEBUG_ERROR*/
#ifdef DEBUG_ERROR
#define PRINTF_ERR(...)	printf(__VA_ARGS__)
#else
#define PRINTF_ERR(...)
#endif

#define RETURN_ERROR(string, ret) \
	do { \
		PRINTF_ERR("%s:%d:" #string, __func__, __LINE__); \
		PRINTF_ERR("\n"); \
		return (ret); \
	} while (0)

#define TYPECAST_TO_TIM(x) timer_private_t *tm = (timer_private_t *)((x))
#define CHECK_RET_AND_TYPECAST(x, y) \
	if ((x) == NULL) { \
		PRINTF_ERR("%s at line %d\n", __func__, __LINE__); \
		return (y); \
	} \
	TYPECAST_TO_TIM(x)

#define CHECK_AND_TYPECAST(x) \
	if ((x) == NULL) { \
		PRINTF_ERR("%s at line %d\n", __func__, __LINE__); \
		return; \
	} \
	TYPECAST_TO_TIM(x)


static TIM_HandleTypeDef tim2;
static TIM_HandleTypeDef tim5;

static bool tim5_init(uint32_t period, uint32_t priority, uint32_t base_freq,
			void *data)
{
	CHECK_RET_AND_TYPECAST(data, false);
	__HAL_RCC_TIM5_CLK_ENABLE();
	tm->timer_handle->Instance = TIM5;
	/* Beduin board SystemCoreClock is 168Mhz. Set the prescalar to 42000*/
	tm->timer_handle->Init.Prescaler = (SystemCoreClock / 2 / base_freq)\
	 - 1;
	tm->timer_handle->Init.CounterMode = TIM_COUNTERMODE_UP;
	/* It will not start until first call to sys_sleep function */
	tm->timer_handle->Init.Period = 0;
	tm->timer_handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	if (HAL_TIM_Base_Init(tm->timer_handle) != HAL_OK)
		RETURN_ERROR("timer 5 init failed", false);
	/* Enable the TIM5 interrupt. */
	HAL_NVIC_SetPriority(TIM5_IRQn, priority, 0);
	HAL_NVIC_EnableIRQ(TIM5_IRQn);
	return true;
}

static bool tim2_init(uint32_t period, uint32_t priority, uint32_t base_freq,
			void *data)
{
	CHECK_RET_AND_TYPECAST(data, false);
	__HAL_RCC_TIM2_CLK_ENABLE();
	tm->timer_handle->Instance = TIM2;
	tm->timer_handle->Init.Prescaler = (SystemCoreClock / 2 / base_freq)\
	 - 1;
	tm->timer_handle->Init.CounterMode = TIM_COUNTERMODE_UP;
	tm->timer_handle->Init.Period = period;
	tm->timer_handle->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	if (HAL_TIM_Base_Init(tm->timer_handle) != HAL_OK)
		RETURN_ERROR("timer 2 init failed", false);

	/* Enable the TIM2 interrupt. */
	HAL_NVIC_SetPriority(TIM2_IRQn, priority, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
	return true;
}

static void tim_reg_callback(timercallback_t cb, void *data)
{
	CHECK_AND_TYPECAST(data);
	tm->cb = cb;
}

static bool tim_is_running(void *data)
{
	CHECK_RET_AND_TYPECAST(data, false);
	if (tm->timer_handle->Instance == NULL)
		RETURN_ERROR("timer instance empty", false);
	return ((tm->timer_handle->Instance)->CR1) & TIM_CR1_CEN;
}

static void tim_start(void *data)
{
	CHECK_AND_TYPECAST(data);
	HAL_TIM_Base_Start_IT(tm->timer_handle);
}

static uint32_t tim5_get_time(void *data)
{
	CHECK_RET_AND_TYPECAST(data, 0);
	return __HAL_TIM_GET_COUNTER(tm->timer_handle) / 2;
}

static void tim_stop(void *data)
{
	CHECK_AND_TYPECAST(data);
	HAL_TIM_Base_Stop_IT(tm->timer_handle);
}

static void tim_set_time(uint32_t period, void *data)
{
	CHECK_AND_TYPECAST(data);
	__HAL_TIM_SET_AUTORELOAD(tm->timer_handle, period);
	__HAL_TIM_SET_COUNTER(tm->timer_handle, 0);
}


#define TIMER_2_PRIVATE         0
#define TIMER_5_PRIVATE         1

static timer_private_t timers[] = {
		[TIMER_2_PRIVATE] = {
		.timer_handle = &tim2,
		.cb = NULL,
		.id = TIMER2,
	},
	[TIMER_5_PRIVATE] = {
		.timer_handle = &tim5,
		.cb = NULL,
		.id = TIMER5
	}
};

void TIM2_IRQHandler(void)
{
	HAL_TIM_IRQHandler(timers[TIMER_2_PRIVATE].timer_handle);
}

void TIM5_IRQHandler(void)
{
	HAL_TIM_IRQHandler(timers[TIMER_5_PRIVATE].timer_handle);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	for (uint8_t i = 0; i < ARRAY_SIZE(timers); i++) {
		if ((timers[i].timer_handle == htim) && timers[i].cb) {
			timers[i].cb();
			break;
		}
	}
}

static const timer_interface_t timer_interface[] = {
	[TIMER_2_PRIVATE] = {
		.init_timer = tim2_init,
		.reg_callback = tim_reg_callback,
		.is_running = tim_is_running,
		.start = tim_start,
		.get_time = NULL,
		.stop = tim_stop,
		.set_time = tim_set_time,
		.irq_handler = NULL,
		.data = &timers[TIMER_2_PRIVATE]
	},
	[TIMER_5_PRIVATE] = {
		.init_timer = tim5_init,
		.reg_callback = tim_reg_callback,
		.is_running = tim_is_running,
		.start = tim_start,
		.get_time = tim5_get_time,
		.stop = tim_stop,
		.set_time = tim_set_time,
		.irq_handler = NULL,
		.data = &timers[TIMER_5_PRIVATE]
	}
};

const timer_interface_t *timer_get_interface(timer_id_t tim)
{
	if (tim < TIMER1 || tim >= MAX_TIMERS)
		return NULL;
	for (uint8_t i = 0; i < ARRAY_SIZE(timer_interface); i++) {
		if (timer_interface[i].data != NULL) {
			timer_private_t *tm = (timer_private_t *)
				(timer_interface[i].data);
			if (tm->id == tim)
				return &timer_interface[i];
		}
	}
	return NULL;
}
