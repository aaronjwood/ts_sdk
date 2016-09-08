#!/bin/bash
# Copyright(C) 2016 Verizon. All rights reserved
# Script to set up tools needed for building the firmware for the STM32F4

# Name of the toolkits from mirror server
TOOLCHAIN_NAME="gcc-arm-none-eabi-4_9-2015q3-20150921-mac.tar.bz2"
STM32F4_CUBE_LIB="stm32cubef4-v1.13.0.zip"
OPENOCD_CONFIG="openocdcfg-stm32f4.zip"

ST_SCRIPT="tools/scripts/stm32f4-bootstrap.sh"
TOOLS_ROOT="$PWD/tools/installed/stm32f4"
STM32F4_CFG_SYS="/usr/local/share/openocd/scripts/target/stm32f4x.cfg"

# Mirror server where toolkit resides
ST_TOOLS_MIRROR="thingservice.verizon.com:/home/vagrant"

if [ $# -ne 1 ]; then
	echo "Provide username for the thingservice.verizon.com mirror"
	echo "Usage: $0 <username>"
	exit 1
fi


if ! [ -f $ST_SCRIPT ]; then
	echo "Script must be run from the root of the git tree."
	exit 1
fi

# See if openocd is installed, first check if brew is installed
BREW_INSTALL=$(which brew)
OPENOCD_INSTALL=$(which openocd)

if [ -z "$OPENOCD_INSTALL" ]; then
	echo "[Installing openocd]"
	if [ -z "$BREW_INSTALL" ]; then
		echo "Install homebrew first from http://brew.sh/"
		echo "Installation not successful..exiting"
		exit 1
	fi
	brew install --HEAD openocd --enable_stlink --with-libusb
	if [ "$?" = "0" ]; then
		echo "brew intalled openocd successfully"
	else
		echo "brew failed to install openocd..exiting"
		exit 1
	fi
	echo "[Installed openocd at /user/local/bin]"
	if ! [ -f $STM32F4_CFG_SYS ]; then
		echo "OpenOCD target configuration file does not exist."
		exit 1
	fi
	echo "[PATH environment variable should append /usr/local/bin]"
fi

echo "[Installing openocd configs]"
	scp $1@$ST_TOOLS_MIRROR/$OPENOCD_CONFIG .
	unzip -d $TOOLS_ROOT/ $OPENOCD_CONFIG
	rm -f $OPENOCD_CONFIG
echo "[Installed openocd configs at $TOOLS_ROOT]"

if [ -d $TOOLS_ROOT ]; then
	echo "stm32f4 tools are already installed for this repo at:"
	echo " $TOOLS_ROOT"
	read -p "Replace? [N]: " ans

	if [ "$ans" != "Y" ] && [ "$ans" != "y" ]; then
		echo "Keeping existing tools."
		echo "[Done]"
		exit 0
	fi
	echo "[Removing previous tools from $TOOLS_ROOT]"
	rm -rf $TOOLS_ROOT
fi

echo "[Creating $TOOLS_ROOT directory]"
mkdir -p $TOOLS_ROOT

echo "[Installing toolchain....]"
	scp $1@$ST_TOOLS_MIRROR/$TOOLCHAIN_NAME .
	tar -C $TOOLS_ROOT/ -x -j -f $TOOLCHAIN_NAME
	rm -f $TOOLCHAIN_NAME
echo "[Toolchain installed at $TOOLS_ROOT]"

echo "[Installing stm32cubef4-v1.13.0 library....]"
	scp $1@$ST_TOOLS_MIRROR/$STM32F4_CUBE_LIB .
	unzip -d $TOOLS_ROOT/ $STM32F4_CUBE_LIB
	rm -f $STM32F4_CUBE_LIB
echo "[library installed at $TOOLS_ROOT]"

echo "[Done]"
