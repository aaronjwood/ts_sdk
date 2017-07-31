/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stm32l4xx_hal.h>
#include "hwrng_hal.h"

static RNG_HandleTypeDef hwrng_handle;

static uint32_t last_num;

void HAL_RNG_MspInit(RNG_HandleTypeDef *hrng)
{
	__HAL_RCC_RNG_CLK_ENABLE();
	RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RNG;
	PeriphClkInitStruct.RngClockSelection = RCC_RNGCLKSOURCE_PLL;
	HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

void HAL_RNG_MspDeInit(RNG_HandleTypeDef *hrng)
{
	__HAL_RCC_RNG_FORCE_RESET();
	__HAL_RCC_RNG_RELEASE_RESET();
}

bool hwrng_module_init(void)
{
	hwrng_handle.Instance = RNG;	/* Point handle at the RNG registers */

	if (HAL_RNG_DeInit(&hwrng_handle) != HAL_OK)
		return false;

	if (HAL_RNG_Init(&hwrng_handle) != HAL_OK)
		return false;

	if (HAL_RNG_GenerateRandomNumber(&hwrng_handle, &last_num) != HAL_OK)
		return false;

	return true;
}


bool hwrng_read_random(uint32_t *pnum)
{
	uint32_t num;

	if (HAL_RNG_GenerateRandomNumber(&hwrng_handle, &num) != HAL_OK)
		return false;

	/* FIPS 140-2 tests that no two adjacent numbers are the same. */
	if (num == last_num)
		return false;

	last_num = num;
	*pnum = num;
	return true;
}
