/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include "timer_tim2.h"

/*
 * XXX: TIM2 priority must be lower than USART2 (serial port between the modem
 * and the MCU) for TS-SDK to work correctly.
 */
#define TIM_IRQ_PRIORITY	6

#define TIM_BASE_FREQ_HZ	1000000

static timercallback_t callback;
static TIM_HandleTypeDef tim2;

static bool tim2_init_period(uint32_t period)
{
	/*
	 * Initialize the timer (TIM2) to have a period equal to the time it
	 * takes to receive a character at the current baud rate.
	 * TIM2's clock source is connected to APB1. According to the TRM,
	 * this source needs to be multiplied by 2 if the APB1 prescaler is not
	 * 1 (it's 4 in our case). Therefore effectively, the clock source for
	 * TIM2 is:
	 * APB1 x 2 = SystemCoreClock / 4 * 2 = SystemCoreClock / 2 = 90 MHz.
	 * The base frequency of the timer's clock is chosen as 1 MHz (time
	 * period of 1 microsecond).
	 * Therefore, prescaler = 90 MHz / 1 MHz - 1 = 89.
	 * The clock is set to restart every 70us (value of 'Period' member)
	 * which is how much time it takes to receive a character on a 115200 bps
	 * UART.
	 */
	__HAL_RCC_TIM2_CLK_ENABLE();
	tim2.Instance = TIM2;
	tim2.Init.Prescaler = SystemCoreClock / 2 / TIM_BASE_FREQ_HZ - 1;
	tim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	tim2.Init.Period = period - 1;
	tim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	if (HAL_TIM_Base_Init(&tim2) != HAL_OK)
		return false;

	/* Enable the TIM2 interrupt. */
	HAL_NVIC_SetPriority(TIM2_IRQn, TIM_IRQ_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
	return true;
}

static void tim2_reg_callback(timercallback_t cb)
{
	callback = cb;
}

static bool tim2_is_running(void)
{
	return ((TIM_TypeDef *)TIM2)->CR1 & TIM_CR1_CEN;
}

static void tim2_start(void)
{
	HAL_TIM_Base_Start_IT(&tim2);
}

static void tim2_stop(void)
{
	HAL_TIM_Base_Stop_IT(&tim2);
}

void TIM2_IRQHandler(void)
{
	HAL_TIM_IRQHandler(&tim2);
}

/*
void tim2_irq_handler(void)
{
	HAL_TIM_IRQHandler(&tim2);
}
*/

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (!callback)
		callback();
}

static const timer_interface_t tim2_interface = {
	.init_period = tim2_init_period,
	.reg_callback = tim2_reg_callback,
	.is_running = tim2_is_running,
	.start = tim2_start,
	.stop = tim2_stop,
	.irq_handler = NULL
};

const timer_interface_t *timer_tim2_get_interface(void)
{
	return &tim2_interface;
}
