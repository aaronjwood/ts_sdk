#!/bin/sh
# Copyright(C) 2017 Verizon. All rights reserved
PROJ_ROOT="$PWD"
TOOLS_ROOT="$PROJ_ROOT/tools/docker/"
SDK_ROOT="$PROJ_ROOT/sdk/cloud_comm/"

BUILD_APP_ARG=""
BUILD_APP_PROTO=""

SCRIPT_NAME=`basename "$0"`
SCRIPT="tools/scripts/$SCRIPT_NAME"
BASE_DOCKERFILE=""

usage()
{
	cat << EOF
Usage: $SCRIPT_NAME <relative path to CHIPSET_DOCKERFILE> <relative path to APP_DOCKERFILE> [Application options]
Example Usage:
tools/scripts/docker_build.sh ./tools/docker/Dockerfile.thingservice ./sample_apps/Dockerfile PROTO=SMSNAS_PROTOCOL APP=sensor_examples

Description:
	Script to build various apps from sample_apps and module_tests directories based on
	application dockerfile provided, script first creates chipset docker image which
	copies all the necessary chipset specific SDK at /ts_sdk_bldenv in container. It
	then creates docker image using image created in first step for the thing space SDK, image
	has all the necessary sdk files at /ts_sdk. In a final step it creates application
	docker image from the image created in second step which has application directory
	at /ts_sdk/sample_apps or /ts_sdk/module_tests and other vendor dependent libraries
	like mbedtls at /ts_sdk/vendor

CHIPSET_DOCKERFILE: relative path to chipset dockerfile, currently two dockerfiles are included in this SDK
	as below:
		1) ts_sdk/tools/docker/Dockerfile.dockerhub -> Fetches base ubuntu image with arm toolchain
		from dockerhub (Use this file when internal server is not available) and
		copies chipset sdk at /ts_sdk_bldenv/ in the docker image
		2) ts_sdk/tools/docker/Dockerfile.thingservice -> Fetches base ubuntu image with arm toolchain
		from internal docker registry and copies chipset sdk at /ts_sdk_bldenv/ in the docker image

APP_DOCKERFILE: relative path to application dockerfile
	This should provide makefile to compile application by linking to /ts_sdk_bldenv
	directory for chipset related sources and header and /ts_sdk_bldenv/toolchain/
	for gcc toolchain, it should also include cc_sdk.mk to link it to sdk sources and header

Application options: Optional paramters which are provided as key=value pair and being used
	as environment variable (-e flag) during docker run command. This parameters
	are compile time configuration parameters for the application. Available options are as below:
	1) APP=<application to compile>, for example APP=cc_test
	available options are any app or test directories from sample_apps and module_tests
	Note: This application should correspond to APP_DOCKERFILE
	2) PROTO=<Communication protocol to use>, thingspace sdk supports two options
	OTT_PROTOCOL and SMSNAS_PROTOCOL as a value

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
				*)
			esac
		fi
	done
}

if ! [ -f $SCRIPT ]; then
	echo "Script must be run from the root of the repository"
	usage
fi

if [ "$#" -eq "0" ]; then
	echo "Specify required arguments"
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
	process_app_args "$3" "$4"
	echo "docker runtime env args: $BUILD_APP_PROTO $BUILD_APP_ARG"
fi

APP_DIR_PATH=`dirname $2`
APP_DIR=`basename $APP_DIR_PATH`

echo "Project root: $PROJ_ROOT"
echo "SDK root: $SDK_ROOT"
echo "Tool root: $TOOLS_ROOT"
echo "Application to build: $APP_DIR"

# Three steps build starts from here
docker build -t stm32f429_buildenv -f $1 $PROJ_ROOT
docker build -t ts_sdk -f $SDK_ROOT/Dockerfile $PROJ_ROOT
docker build -t $APP_DIR -f $2 $PROJ_ROOT
docker run --rm $BUILD_APP_PROTO $BUILD_APP_ARG -v $PROJ_ROOT/build:/build $APP_DIR
