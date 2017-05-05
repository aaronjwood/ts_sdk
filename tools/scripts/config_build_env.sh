#!/bin/sh
# Copyright(C) 2017 Verizon. All rights reserved

SCRIPT_NAME="config_build_env.sh"
SCRIPT="tools/scripts/$SCRIPT_NAME"
ENVFILE=".env_list"
SDK="cloud_comm"
PROJ_ROOT="$PWD"

export PROJ_ROOT=$PROJ_ROOT
export PLATFORM_HAL_ROOT=$PROJ_ROOT/platform
export SDK_ROOT=$PROJ_ROOT/sdk/$SDK
export SDK_APP_ROOT=$PROJ_ROOT/apps

# make build utility makefile fragement to help gather all the object codes into
# application/build directory
export MK_HELPER_PATH=$PROJ_ROOT/tools/config

# chipset and development board related defaults
CHIPSET_FAMILY="stm32f4"
CHIPSET_MCU="stm32f429"
DEV_BOARD="nucleo"

# Defines which modem to use. Currently only the ublox toby201 is supported.
MODEM_TARGET="toby201"

# Defines which cloud protocol to use. Valid options are:
# OTT_PROTOCOL
# SMSNAS_PROTOCOL
PROTOCOL="OTT_PROTOCOL"

usage()
{
	cat << EOF
Description:
	Script to prepare environment for various application build process using
	this sdk. Generated environment variables will be written in $ENVFILE
	file as key value pair and also exported so that make and docker can use
	them in the build process. Script exports PROJ_ROOT, SDK_ROOT, SDK_APP_ROOT
	and PLATFORM_HAL_ROOT as a default, where PROJ_ROOT is root of the repository,
	SDK_ROOT is the root of the SDK, SDK_APP_ROOT is the apps directory
	and PLATFORM_HAL_ROOT is the platform layer root. Beside
	that it also exports $MK_HELPER_PATH for a build utility related includes

Usage:  source $SCRIPT_NAME [Script Options] [Build Options]
        [] - optional and depends on the application build specific environment.
        Specify as a key=value pair.

	Script Options: Script specific options, this does not affect build
	environment.

	print-env:
                Prints currently configured build environment
	clear-env:
		Clears previously exported environment, deletes $ENVFILE as well
	help:
		Displays script usage

	Build Options: List of build options, all the options mentioned here has
	default value set for stm32f429 MCU based nucleo board, ublox toby201
	LTE modem and OTT as a transport protocol. All the options must be
	specified as a key=value pair, for exaple, CHIPSET_MCU=stm32f429.

	CHIPSET_FAMILY: Chipset family, default is stm32f4

	CHIPSET_MCU: Micro controller, dafault value is stm32f429

	DEV_BOARD: Name of the development board, default is nucleo

	MODEM_TARGET: LTE modem target, default is toby201

	PROTOCOL: Cloud data transport protocol, default is OTT_PROTOCOL. Valid
	values are OTT_PROTOCOL and SMSNAS_PROTOCOL
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
			CHIPSET_FAMILY)	CHIPSET_FAMILY="$value";;
			CHIPSET_MCU)	CHIPSET_MCU="$value";;
			DEV_BOARD)	DEV_BOARD="$value";;
			MODEM_TARGET)	MODEM_TARGET="$value";;
			PROTOCOL)	PROTOCOL="$value";;
			*)		usage;;
		esac
	done
}

clear_env()
{
	if [ -f "$ENVFILE" ]; then
	        echo "Clearing current build environment"
	        rm -rf $ENVFILE
	fi
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

}

if ! [ -f "$SCRIPT" ]; then
	echo "Script must be run from the root of the repository"
	return 1
fi

if ! [ -z "$1" ]; then
	if [ "$1" == "print-env" ]; then
		cat $ENVFILE
		return 0
	elif [ "$1" == "clear-env" ]; then
		clear_env
		return 0
	elif [ "$1" == "help" ]; then
		usage
	else
		process_app_args "$@"
	fi
fi

# write in .env_list file
cat > $ENVFILE << EOF
PROJ_ROOT=$PROJ_ROOT
PLATFORM_HAL_ROOT=$PLATFORM_HAL_ROOT
SDK_ROOT=$SDK_ROOT
SDK_APP_ROOT=$SDK_APP_ROOT
MK_HELPER_PATH=$MK_HELPER_PATH
CHIPSET_FAMILY=$CHIPSET_FAMILY
CHIPSET_MCU=$CHIPSET_MCU
DEV_BOARD=$DEV_BOARD
MODEM_TARGET=$MODEM_TARGET
PROTOCOL=$PROTOCOL
EOF

export CHIPSET_FAMILY=$CHIPSET_FAMILY
export CHIPSET_MCU=$CHIPSET_MCU
export DEV_BOARD=$DEV_BOARD
export MODEM_TARGET=$MODEM_TARGET
export PROTOCOL=$PROTOCOL
