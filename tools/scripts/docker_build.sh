#!/bin/sh
# Copyright(C) 2017 Verizon. All rights reserved
PROJ_ROOT="$PWD"
SDK_ROOT="$PROJ_ROOT/sdk/cloud_comm/"
TOOLS_ROOT="$PROJ_ROOT/tools/config/openocd-configs/"
BUILD_DIR="$PROJ_ROOT/build"
FIRMWARE=""
BUILD_APP_ARG=""
BUILD_APP_PROTO=""

SCRIPT_NAME=`basename "$0"`
SCRIPT="tools/scripts/$SCRIPT_NAME"
BASE_DOCKERFILE=""

CHIPSET_IMAGE_NAME="stm32f429_buildenv"
TS_SDK_IMAGE_NAME="ts_sdk"

usage()
{
	cat << EOF
Usage: $SCRIPT_NAME <relative path to CHIPSET_DOCKERFILE> <relative path to APP_DOCKERFILE> \
[Application options]
Note: arguments in <> are mandatory while [] are optional, \
optional arguments has to be specified by key=value format

Example Usage:
tools/scripts/docker_build.sh ./tools/docker/Dockerfile.thingservice ./apps/Dockerfile \
PROTO=SMSNAS_PROTOCOL APP=sensor_examples upload=<relative path to firmware executable>

Description:
	Script to build various apps from apps and module_tests directories based on
	application dockerfile provided, script first creates chipset docker image which
	copies all the necessary chipset specific SDK at /ts_sdk_bldenv in container. It
	then creates docker image using image created in first step for the thing space SDK, image
	has all the necessary sdk files at /ts_sdk. In a final step it creates application
	docker image from the image created in second step which has application directory
	at /ts_sdk/apps or /ts_sdk/module_tests and other vendor dependent libraries
	like mbedtls at /ts_sdk/vendor. Script and eventually docker run command creates
	volume build/<app>/ to house all the compiled objects and binary images. Optional
	upload argument can be supplied to burn given firmware executable

CHIPSET_DOCKERFILE: relative path to chipset dockerfile, currently two dockerfiles are included in this SDK
	as below:
	1) ts_sdk/tools/docker/Dockerfile.dockerhub -> Fetches base ubuntu image with arm toolchain
	from dockerhub (Use this file when internal server is not available) and
	copies chipset sdk at /ts_sdk_bldenv/ in the docker image
	2) ts_sdk/tools/docker/Dockerfile.thingservice -> Fetches base ubuntu image with arm toolchain
	from internal docker registry and copies chipset sdk at /ts_sdk_bldenv/ in the docker image, file
	can be used for any one having access to internal verizon registry

APP_DOCKERFILE: relative path to application dockerfile
	This should provide makefile to compile application by linking to /ts_sdk_bldenv
	directory for chipset related sources and header and /ts_sdk_bldenv/toolchain/
	for gcc toolchain, it should also include cc_sdk.mk to link it to sdk sources and header

Application options: Optional paramters which are provided as key=value
	Available options are as below:

	1) APP=<application to compile>, for example APP=cc_test
	available options are any app or test directories from apps and module_tests
	Note: This application should correspond to APP_DOCKERFILE, default is cc_test
	2) PROTO=<Communication protocol to use>, thingspace sdk supports two options
	OTT_PROTOCOL and SMSNAS_PROTOCOL as a value, default is SMSNAS_PROTOCOL
	Both 1 and 2 options above are used as environment (-e flag) variable in docker run command
	3) upload=<relative path to firmware executable>, this parameter is optional.
	Built executable's name is in firmware_<protocol_name>.elf form, where
	protocol_name is either OTT_PROTOCOL or SMSNAS_PROTOCOL

EOF
	exit 1
}

check_for_dir()
{
	if [ -d $1 ]; then
		echo "Specify dockerfile name along with the relative path to application directory"
		usage
	fi
}

process_app_args()
{
	for ARGUMENT in "$@"
	do
		KEY=$(echo $ARGUMENT | cut -f1 -d=)
		VALUE=$(echo $ARGUMENT | cut -f2 -d=)
		if ! [ -z "$VALUE" ]; then
			case "$KEY" in
				APP)	BUILD_APP_ARG="-e ${KEY}=${VALUE}";;
				PROTO)	BUILD_APP_PROTO="-e ${KEY}=${VALUE}";;
				upload) FIRMWARE="${VALUE}";;
				*)
			esac
		fi
	done
}

cleanup_docker_images()
{
	if [ -z "$1" ]; then
		echo "Cleaning images: $APP_DIR, $TS_SDK_IMAGE_NAME, $CHIPSET_IMAGE_NAME"
		docker rmi -f $APP_DIR
		docker rmi -f $TS_SDK_IMAGE_NAME
		docker rmi -f $CHIPSET_IMAGE_NAME
	else
		echo "Cleaning image: $1"
		docker rmi -f $1
	fi
}

cleanup_prev_docker_builds()
{
	if [ -d $BUILD_DIR ]; then
		echo "Performing clean ups before docker build"
		rm -rf $BUILD_DIR
	fi
}

upload_firmware()
{
	if ! [ -f $FIRMWARE ]; then
		echo "Provide valid firmware executable file"
	else
		echo "File to upload: $FIRMWARE"
		openocd -f $TOOLS_ROOT/stm32f4/board/st_nucleo_f4.cfg \
			-c init -c "reset halt" \
			-c "flash write_image erase $FIRMWARE" \
			-c reset -c shutdown
	fi
}

if ! [ -f $SCRIPT ]; then
	echo "Script must be run from the root of the repository"
	usage
fi

if [ "$#" -eq "0" ]; then
	echo "Insufficient arguments"
	usage
fi

if [ -z "$1" ]; then
	echo "Specify relative path including dockerfile name for the chipset base image to use"
	usage
else
	check_for_dir "$1"
fi

if [ -z "$2" ]; then
	echo "Specify relative path including dockerfile name in the application directory to use"
	usage
else
	check_for_dir "$2"
	process_app_args "$3" "$4" "$5"
	echo "docker runtime env args: $BUILD_APP_PROTO $BUILD_APP_ARG"
fi

APP_DIR_PATH=`dirname $2`
APP_DIR=`basename $APP_DIR_PATH`

echo "Project root: $PROJ_ROOT"
echo "SDK root: $SDK_ROOT"
echo "Application to build: $APP_DIR"

cleanup_prev_docker_builds

# Three steps build starts from here
docker build -t $CHIPSET_IMAGE_NAME -f $1 $PROJ_ROOT
docker build -t $TS_SDK_IMAGE_NAME -f $SDK_ROOT/Dockerfile $PROJ_ROOT
docker build -t $APP_DIR -f $2 $PROJ_ROOT
docker run --rm $BUILD_APP_PROTO $BUILD_APP_ARG -v $PROJ_ROOT/build:/build $APP_DIR

if [ $? -eq 0 ]; then
	echo "Build was success"
	if ! [ -z $FIRMWARE ]; then
		echo "Uploading firmware..."
		upload_firmware
	fi
else
	echo "Build failed"
	cleanup_docker_images
	exit 1
fi

cleanup_docker_images
