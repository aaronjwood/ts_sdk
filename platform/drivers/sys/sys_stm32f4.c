/* Copyright(C) 2016, 2017 Verizon. All rights reserved. */

#include <stm32f4xx_hal.h>
#include <dbg.h>
#include "timer_hal.h"
#include "timer_interface.h"
#include "ts_sdk_config.h"

/* Timer related definations */
static const timer_interface_t *sleep_timer;
static volatile bool timer_expired;
static uint32_t rem_sleep;
/* total time in milliseconds CPU spent in sleep */
static uint64_t total_sleep_time;
#define TIM5_IRQ_PRIORITY	7	/* TIM5 priority is lower than TIM2 */
#define TIM5_BASE_FREQ_HZ	2000
/* Maximum timeout period it can be programmed for in miliseconds */
#define MAX_TIMER_RES_MS	(uint32_t)2147483648
/* End of timer defination */

/* Sys tick counter definations */
static uint32_t base_tick;
static uint64_t accu_tick;
/* End of sys tick counter defination */

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 180000000
  *            HCLK(Hz)                       = 180000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 8
  *            PLL_N                          = 360
  *            PLL_P                          = 2
  *            PLL_Q                          = 8
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* Enable Power Control clock */
	__HAL_RCC_PWR_CLK_ENABLE();

	/*
	 * The voltage scaling allows optimizing the power consumption when the
	 * device is clocked below the maximum system frequency, to update the
	 * voltage scaling value regarding system frequency refer to product
	 * datasheet.
	 */
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/*
	 * Enable HSE Oscillator and activate PLL with HSE as source.
	 * Note : The following code uses a non-crystal / non-ceramic high speed
	 * external clock source (signified by RCC_HSE_BYPASS). It is derived
	 * from the MCO of the chip housing the STLink firmware. This MCO is in
	 * turn sourced from an 8MHz on-board crystal.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 360;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 8;		/* PLL48CLK = 45 MHz. */
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
		/* Initialization Error */
		raise_err();

	/* Extend maximum clock frequency to 180 MHz from 168 MHz. */
	if(HAL_PWREx_EnableOverDrive() != HAL_OK)
		/* Initialization Error */
		raise_err();

	/*
	 * Select PLL as system clock source and configure the HCLK, PCLK1 and
	 * PCLK2 clocks dividers.
	 */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK |
			RCC_CLOCKTYPE_HCLK |
			RCC_CLOCKTYPE_PCLK1 |
			RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK; /* 180 MHz */
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;        /* 180 MHz */
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;         /* 45 MHz */
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;         /* 90 MHz */
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
		/* Initialization Error */
		raise_err();
}

static void init_reset_gpio(void)
{
	/*
	 * Port D Pin 2 (Pin no. 12 on the CN8 header of the Nucleo board) is
	 * designated as the reset control pin. The modem is reset through
	 * RESET_N which is the 26th pin on the DIL B2B J300 connector.
	 */
	/* XXX: The GPIO pin settings might be different for another modem. */
	GPIO_InitTypeDef reset_pins;
	__HAL_RCC_GPIOD_CLK_ENABLE();
	reset_pins.Pin = GPIO_PIN_2;
	reset_pins.Mode = GPIO_MODE_OUTPUT_OD;
	reset_pins.Pull = GPIO_NOPULL;
	reset_pins.Speed = GPIO_SPEED_FREQ_HIGH;
	HAL_GPIO_Init(GPIOD, &reset_pins);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
}

static void tim5_cb(void)
{
	timer_expired = true;
}

/* Set up timer module 5 (TIM5) to facilitate sleep functionality */
static bool tim5_module_init(void)
{
	/* Timer 5's prescaler is fed by APB clock which is 90MHz, setting
	 * precaler to highest divider possible to make generate counter clock
	 * 90MHz / 45000 = base_frequency = 2KHz = 0.5mS
	 * Minumum resolution = 0.5mS
	 * Maximum resolution = 0.5mS * Auto Reload register (Period) value
	 */

	timer_expired = false;
	rem_sleep = 0;
	total_sleep_time = 0;
	sleep_timer = timer_get_interface(SLEEP_TIMER);

	if (!sleep_timer)
		return false;
	return timer_init(sleep_timer, 0, TIM5_IRQ_PRIORITY,
				TIM5_BASE_FREQ_HZ, tim5_cb);
}

void sys_init(void)
{
	HAL_Init();
	SystemClock_Config();
	init_reset_gpio();

	if (!tim5_module_init())
		dbg_printf("Timer module init failed\n");
	base_tick = 0;
	accu_tick = 0;
}

void sys_delay(uint32_t delay_ms)
{
        HAL_Delay(delay_ms);
}

uint64_t sys_get_tick_ms(void)
{
	uint32_t cur_ts = HAL_GetTick();
	uint32_t diff_ms = 0;
	if (cur_ts >= base_tick)
		diff_ms = cur_ts - base_tick;
	else
		diff_ms = 0xffffffff - base_tick + cur_ts;

	accu_tick += diff_ms;
	base_tick = cur_ts;
	return accu_tick + total_sleep_time;
}

static uint32_t set_timer(uint32_t sleep)
{
	if (sleep > MAX_TIMER_RES_MS) {
		rem_sleep = sleep - MAX_TIMER_RES_MS;
		sleep = MAX_TIMER_RES_MS;
	} else
		rem_sleep = 0;
	timer_set_time(sleep_timer, sleep * 2);
	timer_start(sleep_timer);
	return sleep;
}

/* Returns remaining time left if sleep was interrupted */
static uint32_t handle_wakeup_event(uint32_t sleep)
{
	uint32_t slept_till = timer_get_time(sleep_timer);
	timer_stop(sleep_timer);
	/* means it woke up from some other source */
	if (!timer_expired)
		rem_sleep = 0;
	else {
		slept_till = sleep;
		timer_expired = false;
	}
	return slept_till;
}

uint32_t sys_sleep_ms(uint32_t sleep)
{
	if (sleep == 0)
		return 0;
	uint32_t total_slept = 0;
	uint32_t sleep_temp = 0;
	/* Disable systick so that processor does not wake up every 1ms */
	HAL_SuspendTick();
	/* Request to enter SLEEP mode */
	do {
		sleep_temp = set_timer(sleep);
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
		total_slept += handle_wakeup_event(sleep_temp);
		sleep = sleep - total_slept;
	} while (rem_sleep);

	total_sleep_time += total_slept;
	/* Resume systick interrupt */
	HAL_ResumeTick();
	return sleep;
}

/* Increments the SysTick value. */
void SysTick_Handler(void)
{
	HAL_IncTick();
}

/*
 * The following are interrupt handlers that trap illegal memory access and
 * illegal program behavior. These should be defined to prevent runaway code
 * while debugging.
 */
void HardFault_Handler(void)
{
	while (1)
		;
}

void MemManage_Handler(void)
{
	while (1)
		;
}

void BusFault_Handler(void)
{
	while (1)
		;
}

void UsageFault_Handler(void)
{
	while (1)
		;
}

void sys_reset_modem(uint16_t pulse_width_ms)
{
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_RESET);
	sys_delay(pulse_width_ms);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2, GPIO_PIN_SET);
}
