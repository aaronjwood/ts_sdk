#!/bin/bash
ROOT=/Users/v768213/work/ts_sdk
DRV_ROOT=$ROOT/tools/installed/stm32f4/STM32Cube_FW_F4_V1.13.0/Drivers
DRV_SRC_ROOT=$ROOT/tools/installed/stm32f4/STM32Cube_FW_F4_V1.13.0/Drivers/STM32F4xx_HAL_Driver/Src
arm-none-eabi-gcc --std=c99 -Wall -Wcast-align -Werror --specs=nosys.specs \
	-DSTM32F429xx -mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16 \
	-I $ROOT/include/platform \
	-I $DRV_ROOT/STM32F4xx_HAL_Driver/Inc \
	-I $DRV_ROOT/CMSIS/Device/ST/STM32F4xx/Include \
	-I $DRV_ROOT/CMSIS/Include \
	*.c \
	$DRV_SRC_ROOT/stm32f4xx_hal.c \
	$DRV_SRC_ROOT/stm32f4xx_hal_gpio.c \
	$DRV_SRC_ROOT/stm32f4xx_hal_cortex.c \
	$DRV_ROOT/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c
#gcc --std=c99 -Wall -Wcast-align -Werror main.c
