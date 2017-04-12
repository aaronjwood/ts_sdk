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
	echo "Usage: "
	echo "$SCRIPT_NAME: Script to build various apps from sample_apps and module test directories"
	echo "Provide application or module test name to build as first argument"
}

check_for_dir()
{
	if [ -d $1 ]; then
		echo "Specify dockerfile name along with the relative path to application directory"
		exit 1
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

if [ "$#" -eq "0" ]; then
	echo "Specify required arguments"
	usage
	exit 1
fi

if ! [ -f $SCRIPT ]; then
	echo "Script must be run from the root of the repository"
	usage
	exit 1
fi

if [ -z "$1" ]; then
	echo "Specify relative path including dockerfile name for the chipset base image to use"
	usage
	exit 1
else
	check_for_dir "$1"
fi

if [ -z "$2" ]; then
	echo "Specify relative path including dockerfile name in the application directory to use"
	usage
	exit 1
else
	check_for_dir "$2"
	process_app_args "$3" "$4"
	echo "runtime env args $BUILD_APP_PROTO $BUILD_APP_ARG"
fi

APP_DIR_PATH=`dirname $2`
APP_DIR=`basename $APP_DIR_PATH`

echo "Project root: $PROJ_ROOT"
echo "SDK root: $SDK_ROOT"
echo "Tool root: $TOOLS_ROOT"
echo "Application to build: $APP_DIR"

# Three steps build starts from here
docker volume rm $(docker volume ls -f "dangling=true" -q)
docker build -t stm32f429_buildenv -f $1 $PROJ_ROOT
docker build -t ts_sdk -f $SDK_ROOT/Dockerfile $PROJ_ROOT
docker build -t $APP_DIR -f $2 $PROJ_ROOT
docker run --rm $BUILD_APP_PROTO $BUILD_APP_ARG -v $PROJ_ROOT/build:/build $APP_DIR
