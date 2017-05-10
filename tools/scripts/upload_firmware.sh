#!/bin/bash
# Copyright(C) 2017 Verizon. All rights reserved
FIRMWARE=""
UPLOADER=openocd

TOOLS_ROOT="$PROJ_ROOT/tools/config/openocd-configs"
SCRIPT_NAME=`basename "$0"`
SCRIPT="tools/scripts/$SCRIPT_NAME"

usage()
{
	cat << EOF
Usage: $SCRIPT_NAME <relative path to firmare executable> [Upload Options]

Note: arguments in <> are mandatory while in [] are optional.
Optional arguments has to be specified by key=value format, if not specified,
they will assume default values.

Example Usage:
tools/scripts/upload_firmware.sh <path to firmware executable>

Description:
	Script to upload firmware to target

Upload options: Optional paramters which are provided as key=value
	Available options are as below:

	1) uploader=uploader tool name, this parameter is optional and default value is
        openocd, make sure uploader tool is added in binary search PATH.

EOF
	exit 1
}

if [ -z $PROJ_ROOT ]; then
        echo "Run source tools/scripts/config_build_env.sh help to setup environment"
        exit 1
fi

if ! [ -f $SCRIPT ]; then
	echo "Script must be run from the root of the repository"
	usage
fi

if [ "$#" -eq "0" ]; then
	echo "Insufficient arguments"
	usage
fi

if [ -z "$1" ]; then
	echo "Specify relative path to firmware executable"
	usage
elif [ "$1" = "help" ]; then
	usage
else
        FIRMWARE=$1
fi

upload_firmware_ocd()
{
	if ! [ -f $FIRMWARE ]; then
		echo "Provide valid firmware executable file"
	else
		echo "File to upload: $FIRMWARE"
		$UPLOADER -f $TOOLS_ROOT/stm32f4/board/st_nucleo_f4.cfg \
			-c init -c "reset halt" \
			-c "flash write_image erase $FIRMWARE" \
			-c reset -c shutdown
	fi
}

process_app_args()
{
	if ! [ -z $1 ]; then
		KEY=$(echo $1 | cut -f1 -d=)
		VALUE=$(echo $1 | cut -f2 -d=)
                if ! [ -z "$VALUE" ]; then
			case "$KEY" in
				uploader)   UPLOADER=$VALUE;;
				*)          usage;;
			esac
		fi
        fi
	if [ $UPLOADER == openocd ]; then
        	upload_firmware_ocd
	fi
}

process_app_args "$2"
