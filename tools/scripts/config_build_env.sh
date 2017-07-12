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

# make build utility makefile fragement to help gather all the object codes into
# application/build directory
export MK_HELPER_PATH=$PROJ_ROOT/tools/config

# Example stmicro stm32f4 chipset and development board related parameters
#CHIPSET_FAMILY="stm32f4"
#CHIPSET_MCU="stm32f429zit"
#DEV_BOARD="nucleo"

# Defines which modem to use. Currently only the ublox toby201 is supported.
#MODEM_TARGET="toby201"

# Defines which cloud protocol to use. Valid options are:
# OTT_PROTOCOL
# SMSNAS_PROTOCOL
# NO_PROTOCOL
#PROTOCOL="OTT_PROTOCOL"

# Defines which GPS chip to use. Currently only neo-6m is supported
# for CHIPSET_MCU = stm32f415rgt.
# GPS_CHIPSET="neo-6m"

usage()
{
	cat << EOF
Description:
	Script to prepare environment for various application build process using
	this sdk. Generated environment variables will exported so that make and
	docker can use them in the build process. Script exports PROJ_ROOT,
	SDK_ROOT, SDK_APP_ROOT, PLATFORM_HAL_ROOT and MK_HELPER_PATH as a default,
	where PROJ_ROOT is root of the repository,
	SDK_ROOT is the root of the SDK, SDK_APP_ROOT is the apps directory
	PLATFORM_HAL_ROOT is the platform layer root,
	MK_HELPER_PATH is path to tools/config to find various makefile helpers.

	Note: This script needs to be sourced both in host and docker container before
	executing any build related scripts. In the docker build process, sourcing
	scripts sets Above mentioned various repository paths which helps makefile
	to find different sources.

Example Usage:
source $SCRIPT_NAME chipset_family=<value> mcu=<> dev_board=<> modem=<> protocol=<>
gps_chipset=<>

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

	modem: LTE modem target, only toby201 value supported

	protocol: Cloud data transport protocol. Valid values are OTT_PROTOCOL,
	SMSNAS_PROTOCOL and NO_PROTOCOL

	gps_chipset: Name of the GPS chipset. Currently only neo-6m value is supported.

	Note: All the options must be present, there are no default values

EOF
	return 0
}

process_app_args()
{
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
			gps_chipset)    GPS_CHIPSET="$value";;	
			*)		usage;;
		esac
	
	done

}

clear_env()
{
	unset CHIPSET_FAMILY
	unset CHIPSET_MCU
	unset DEV_BOARD
	unset MODEM_TARGET
	unset PROTOCOL
	unset PROJ_ROOT
	unset PLATFORM_HAL_ROOT
	unset SDK_ROOT
	unset MK_HELPER_PATH
	unset SDK_APP_ROOT
	unset GPS_CHIPSET

}

print_env()
{
	echo "Chipset family = $CHIPSET_FAMILY"
	echo "Chipset MCU = $CHIPSET_MCU"
	echo "Dev board/kit = $DEV_BOARD"
	echo "Modem target = $MODEM_TARGET"
	echo "Cloud transport protocol = $PROTOCOL"
	echo "GPS Chipset = $GPS_CHIPSET"
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
