#!/bin/bash
# Copyright(C) 2017 Verizon. All rights reserved.
# This scripts update the compilation flags in Truestudio's .cproject file based on chipset.
if [ $3 == 'stm32l476rgt' ]
then 	
gsed -i '/STM32L476xx"\/>/e cat '$1'' $2
elif [ $3 == 'stm32f415rgt' ]
then
gsed -i '/STM32F415xx"\/>/e cat '$1'' $2
elif [ $3 == 'stm32f429zit' ]
then
gsed -i '/STM32F429xx"\/>/e cat '$1'' $2
else
echo "Invalid chipset"
fi

exit 0
