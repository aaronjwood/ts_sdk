#!/bin/bash
# Copyright(C) 2016,2017 Verizon. All rights reserved
# Script to set up tools needed for building the firmware for the STM32F4

# Toolkit files on the internal mirror server
TOOLCHAIN_NAME="gcc-arm-none-eabi-5_4-2016q2-20160622-mac.tar.bz2"
STM32F4_CUBE_LIB="stm32cubef4-v1.13.0.zip"

ST_SCRIPT="tools/scripts/stm32f4-bootstrap.sh"
TOOLS_ROOT="$PWD/tools/installed/stm32f4"
STM32F4_CFG_SYS="/usr/local/share/openocd/scripts/target/stm32f4x.cfg"

# Internal mirror server where toolkit resides
INTERNAL_TOOLS_MIRROR="thingservice.verizon.com:/home/vagrant"

# External source for GCC toolchain
GCC_TOOLS_URL="https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q2-update/+download/$TOOLCHAIN_NAME"


if ! [ -f $ST_SCRIPT ]; then
	echo "Script must be run from the root of the git tree."
	exit 1
fi

# Invoke with <username> parameter to install from the Verizon internal mirror.
USERNAME="$1"
if [ -z "$USERNAME" ]; then
	cat <<-'EOF'
	To setup the STM32F4 build environment, you must first download the
	STM32CubeF4 developement software from the STMicroelectronics site:

	http://www.st.com/content/st_com/en/products/embedded-software/mcus-embedded-software/stm32-embedded-software/stm32cube-embedded-software/stm32cubef4.html

	To do this, you must be logged in with your free myST account.

	EOF
	read -p "Path to downloaded STM32CubeF4 zip file (<return> to exit): " \
		answer
	if [ -z "$answer" ]; then
		exit 0
	fi
	STM32F4_CUBE_LIB="$answer"
	if ! [ -f "$STM32F4_CUBE_LIB" ]; then
		echo "File: $STM32F4_CUBE_LIB not found. Exiting.."
		exit 1
	fi
fi

# See if openocd is installed, first check if brew is installed
BREW_INSTALL=$(which brew)
OPENOCD_INSTALL=$(which openocd)

if [ -z "$OPENOCD_INSTALL" ]; then
	echo "[Installing openocd]"
	if [ -z "$BREW_INSTALL" ]; then
		echo "Installation of openocd is done using homebrew."
		echo "Please install it first from http://brew.sh/."
		echo "Exiting.."
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
	OPENOCD_INSTALL=/usr/local/bin/openocd
fi
OPENOCD_VERSION=$($OPENOCD_INSTALL -v 2>&1 | head -n 1)
echo
echo "[Openocd version is: $OPENOCD_VERSION]"

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
	if [ -z "$USERNAME" ]; then
		wget -O $TOOLCHAIN_NAME $GCC_TOOLS_URL
	else
		scp ${USERNAME}@${INTERNAL_TOOLS_MIRROR}/$TOOLCHAIN_NAME .
	fi
	tar -C $TOOLS_ROOT/ -x -j -f $TOOLCHAIN_NAME
	rm -f $TOOLCHAIN_NAME
echo "[Toolchain installed at $TOOLS_ROOT]"

echo "[Installing openocd configs]"
cp -a $TOOLS_ROOT/../../openocd-configs/stm32f4 $TOOLS_ROOT/openocdcfg-stm32f4
echo "[Installed openocd configs at $TOOLS_ROOT]"

echo "[Installing STM32CubeF4 library....]"
if [ -z "$USERNAME" ]; then
	cp $STM32F4_CUBE_LIB .
else
	scp ${USERNAME}@${INTERNAL_TOOLS_MIRROR}/$STM32F4_CUBE_LIB .
fi
unzip -q -d $TOOLS_ROOT/ $STM32F4_CUBE_LIB
rm -f ./$(basename $STM32F4_CUBE_LIB)
echo "[Library installed at $TOOLS_ROOT]"

DOXYGEN_INSTALL=$(which doxygen)
if [ -z "$DOXYGEN_INSTALL" ]; then
	echo
	echo "If you wish to generate API documentation from the source code"
	echo "install doxygen with the following command:"
	echo "    brew install doxygen"
fi
(echo "$PATH" | egrep -q '^/usr/local/bin:|:/usr/local/bin:|:/usr/local/bin$') ||\
	echo "[Please add /usr/local/bin to your PATH variable.]"
echo "[Done]"
