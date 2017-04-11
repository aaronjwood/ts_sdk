#!/bin/bash
ROOT=/Users/v768213/work/ts_sdk
DRV_ROOT=$ROOT/tools/installed/stm32f4/STM32Cube_FW_F4_V1.13.0/Drivers
arm-none-eabi-gcc --std=c99 -Wall -Wcast-align -Werror --specs=nosys.specs \
	-DSTM32F429xx \
	-I $ROOT/include/platform \
	-I $DRV_ROOT/STM32F4xx_HAL_Driver/Inc \
	-I $DRV_ROOT/CMSIS/Device/ST/STM32F4xx/Include \
	-I $DRV_ROOT/CMSIS/Include \
	*.c $DRV_ROOT/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
#gcc --std=c99 -Wall -Wcast-align -Werror main.c
