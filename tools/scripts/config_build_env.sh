#!/bin/bash
# Copyright(C) 2017 Verizon. All rights reserved

SCRIPT_NAME="config_build_env.sh"
SCRIPT="tools/scripts/$SCRIPT_NAME"
SDK="cloud_comm"
PROJ_ROOT="$PWD"

export PROJ_ROOT=$PROJ_ROOT
export PLATFORM_HAL_ROOT=$PROJ_ROOT/platform
export SDK_ROOT=$PROJ_ROOT/sdk/$SDK
export SDK_APP_ROOT=$PROJ_ROOT/apps
export TARGET_ROOT=$PROJ_ROOT/targets

# make build utility makefile fragement to help gather all the object codes into
# application/build directory
export MK_HELPER_PATH=$PROJ_ROOT/tools/config

# Example stmicro stm32f4 chipset and development board related parameters
#CHIPSET_FAMILY="stm32f4"
#CHIPSET_MCU="stm32f429zit"
#DEV_BOARD="nucleo"

# Example stmicro stm32l4 chipset and development board related parameters
#CHIPSET_FAMILY="stm32l4"
#CHIPSET_MCU="stm32l476rgt"
#DEV_BOARD="nucleo"

# Example raspberry_pi3 development board related parameters
#CHIPSET_FAMILY=""
#CHIPSET_MCU=""
#DEV_BOARD="raspberry_pi3"

# Example linux based virtual devices related parameters
#CHIPSET_FAMILY=""
#CHIPSET_MCU=""
#DEV_BOARD="virtual"
#MODEM_TARGET=none
#PROTOCOL=MQTT_PROTOCOL

# Defines which modem to use. Currently only the ublox toby201 and Sequans Monarch are supported.
# raspberry_pi3 and linux virtual devices do not use any modem
#MODEM_TARGET="toby201" or "sqmonarch" or "none"

# Defines which operating system to use. Currently bare metal and FREE_RTOS is supported.
#CHIPSET_OS="FREE_RTOS"

# Defines which cloud protocol to use. Valid options are:
# OTT_PROTOCOL
# SMSNAS_PROTOCOL
# MQTT_PROTOCOL
# NO_PROTOCOL
#PROTOCOL="OTT_PROTOCOL"

# Defines which GPS chip to use. neo-6m, cxd5600 are supported.
# Currently for CHIPSET_MCU = stm32f415rgt GPS_CHIPSET=neo-6m
# is supoorted and for CHIPSET_MCU = stm32l476rgt GPS_CHIPSET=cxd5600
# is supported.

usage()
{
	cat << EOF
Description:
	Script to prepare environment for various application build process using
	this sdk. Generated environment variables will exported so that make and
	docker can use them in the build process. Script exports PROJ_ROOT,
	SDK_ROOT, SDK_APP_ROOT, TARGET_ROOT, PLATFORM_HAL_ROOT and MK_HELPER_PATH
	as a default, where PROJ_ROOT is root of the repository,
	SDK_ROOT is the root of the SDK, SDK_APP_ROOT is the apps directory
	PLATFORM_HAL_ROOT is the platform layer root,
	TARGET_ROOT is where chipset related source resides,
	MK_HELPER_PATH is path to tools/config to find various makefile helpers.

	Note: This script needs to be sourced both in host and docker container before
	executing any build related scripts. In the docker build process, sourcing
	scripts sets Above mentioned various repository paths which helps makefile
	to find different sources.

Example Usage:
source $SCRIPT_NAME chipset_family=<value> mcu=<> dev_board=<> modem=<> protocol=<>
gps_chipset=<> os=<> host=<> ssl_host=<>

Short Cut Settings: This sets sdk default for chipset family, mcu, board and modem
except transport protocol

source $SCRIPT_NAME sdk_defaults protocol=<>

Usage:  source $SCRIPT_NAME Options
	Options are speficied using key=value pair

	Script Options: Script specific options

	print-env:
                Prints currently configured build environment
	clear-env:
		Clears previously exported environment
	help:
		Displays script usage

	Build Options: List of build options, all the options must be
	specified as a key=value pair, for example, mcu=stm32f429zit.

	chipset_family: Chipset family

	mcu: Micro controller

	dev_board: Name of the development board

	modem: LTE modem target, only "toby201", "sqmonarch" and "none" value supported

	protocol: Cloud data transport protocol. Valid values are OTT_PROTOCOL,
	SMSNAS_PROTOCOL, MQTT_PROTOCOL and NO_PROTOCOL

	gps_chipset: Name of the GPS chipset. Valid values are neo-6m and cxd5600.
	Leave it blank for no gps chipset.

	os: Name of the OS. Currently FREE_RTOS and bare-metal is supported.
	This is optional, Valid values are : FREE_RTOS or leave it blank if no os.

	host: Remote host address for protocol.
	Format to be followed is: '"abc.com:8883"' or '"12.21.13.14:8883"'

	ssl_host: Remote host address for certificate verification.
	Format to be followed is:- '"abc.com"'

	Note: All the options must be present, there are no default values

EOF
	return 0
}

clear_sdk_chip_env()
{
	unset CHIPSET_FAMILY
	unset CHIPSET_MCU
	unset DEV_BOARD
	unset MODEM_TARGET
	unset PROTOCOL
	unset GPS_CHIPSET
	unset CHIPSET_OS
	unset REMOTE_HOST
	unset SSL_HOST
}

clear_env()
{
	clear_sdk_chip_env
	unset PROJ_ROOT
	unset PLATFORM_HAL_ROOT
	unset SDK_ROOT
	unset TARGET_ROOT
	unset MK_HELPER_PATH
	unset SDK_APP_ROOT

}

process_app_args()
{
	clear_sdk_chip_env
	for ARGUMENT in "$@"
	do
		key=$(echo $ARGUMENT | cut -f1 -d=)
		value=$(echo $ARGUMENT | cut -f2 -d=)
		case "$key" in
			chipset_family)	CHIPSET_FAMILY="$value";;
			mcu)		CHIPSET_MCU="$value";;
			dev_board)	DEV_BOARD="$value";;
			modem)		MODEM_TARGET="$value";;
			protocol)	PROTOCOL="$value";;
			gps_chipset)	GPS_CHIPSET="$value";;
			os)		CHIPSET_OS="$value";;
			host)		REMOTE_HOST="$value";;
			ssl_host)	SSL_HOST="$value";;
			*)		usage;;
		esac

	done

}

print_env()
{
	echo "Chipset family = $CHIPSET_FAMILY"
	echo "Chipset MCU = $CHIPSET_MCU"
	echo "Dev board/kit = $DEV_BOARD"
	echo "Modem target = $MODEM_TARGET"
	echo "Cloud transport protocol = $PROTOCOL"
	echo "GPS Chipset = $GPS_CHIPSET"
	echo "Operating System = $CHIPSET_OS"
	echo "Remote Host = $REMOTE_HOST"
	echo "SSL Host = $SSL_HOST"
}

exp_sdk_defaults()
{
	export CHIPSET_FAMILY="stm32f4"
	export CHIPSET_MCU="stm32f429zit"
	export DEV_BOARD="nucleo"
	export MODEM_TARGET="toby201"
	if ! [ -z "$1" ]; then
		key=$(echo $1 | cut -f1 -d=)
		value=$(echo $1 | cut -f2 -d=)
		if [ $key == "protocol" ]; then
			export PROTOCOL="$value"
		fi
	fi
}

if ! [ -f "$SCRIPT" ]; then
	echo "Script must be run from the root of the repository"
	return 1
fi

if [ "$1" = "help" ]; then
	usage
	exit 0
fi

if ! [ -z "$1" ]; then
	if [ "$1" = "print-env" ]; then
		print_env
		return 0
	elif [ "$1" = "clear-env" ]; then
		clear_env
		return 0
	elif [ "$1" = "sdk_defaults" ]; then
		shift
		exp_sdk_defaults "$@"
		return 0
	else
		process_app_args "$@"
	fi
fi

export CHIPSET_FAMILY=$CHIPSET_FAMILY
export CHIPSET_MCU=$CHIPSET_MCU
export DEV_BOARD=$DEV_BOARD
export MODEM_TARGET=$MODEM_TARGET
export PROTOCOL=$PROTOCOL
export GPS_CHIPSET=$GPS_CHIPSET
export CHIPSET_OS=$CHIPSET_OS
export REMOTE_HOST=$REMOTE_HOST
export SSL_HOST=$SSL_HOST
