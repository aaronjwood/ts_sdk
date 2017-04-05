#!/bin/bash
gcc --std=c99 -Wall -Wcast-align -Werror \
	-DSTM32F429xx \
	-I /Users/v768213/work/ott_firmware/include/platform \
	-I /Users/v768213/work/ott_firmware/tools/installed/stm32f4/STM32Cube_FW_F4_V1.13.0/Drivers/STM32F4xx_HAL_Driver/Inc \
	-I /Users/v768213/work/ott_firmware/tools/installed/stm32f4/STM32Cube_FW_F4_V1.13.0/Drivers/CMSIS/Device/ST/STM32F4xx/Include \
	-I /Users/v768213/work/ott_firmware/tools/installed/stm32f4/STM32Cube_FW_F4_V1.13.0/Drivers/CMSIS/Include \
	*.c
#gcc --std=c99 -Wall -Wcast-align -Werror main.c
