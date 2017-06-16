#!/bin/bash
# Copyright (C) 2017 Verizon.  All rights reserved.
# Script to find the HSE clock frequency as per the target platform.
# The Script finds the EXT_CLK_FREQ_HZ defined in file board_Config.h. 
# As per the value in this file, the library will be made with supported HSE value.

CHIPSET_MCU="$1"
if [ $CHIPSET_MCU = stm32f415rgt ]; then
	HSEVALUE=`grep  "EXT_CLK_FREQ_HZ" /ts_sdk_bldenv/stm32f415rgt/beduin/board_config.h  | cut -f3 -d" "`
elif [ $CHIPSET_MCU = stm32f429zit ]; then
	HSEVALUE=`grep  "EXT_CLK_FREQ_HZ" /ts_sdk_bldenv/stm32f429zit/nucleo/board_config.h  | cut -f3 -d" "`
fi
if [ "$HSEVALUE" = "((uint32_t)0)" ]; then
echo 
elif [ "$HSEVALUE" = "((uint32_t)12000000)" ]; then
echo "-DHSE_VALUE=12000000"
elif [ "$HSEVALUE" = "((uint32_t)8000000)" ]; then
echo "-DHSE_VALUE=8000000"
else
echo 
fi
