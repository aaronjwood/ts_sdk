#!/bin/sh
# Copyright(C) 2017 Verizon. All rights reserved

MOUNT_DIR="$PROJ_ROOT/build"
# default mount point for the chipset build environment to output build artifacts
CHIPSET_BUILD_MOUNT="$MOUNT_DIR"

# default mount point for the application build environment to output build artifacts
APP_BUILD_MOUNT="$MOUNT_DIR"

BUILD_APP_ARG=""

SCRIPT_NAME=`basename "$0"`
SCRIPT="tools/scripts/$SCRIPT_NAME"

CHIPSET_IMAGE_NAME="${CHIPSET_MCU}_buildenv"
TS_SDK_IMAGE_NAME="ts_sdk"

usage()
{
	cat << EOF
Usage: $SCRIPT_NAME <relative path to CHIPSET_DOCKERFILE> <relative path to APP_DOCKERFILE> \
[Application options]
Note: arguments in <> are mandatory while [] are optional, \
optional arguments has to be specified by key=value format

Example Usage:
tools/scripts/docker_build.sh ./tools/docker/Dockerfile.thingservice ./sample_apps/Dockerfile \
APP=sensor_examples

Description:
	Script to build various apps from apps and module_tests directories based on
	application dockerfile provided, script first creates chipset docker image which
	copies all the necessary chipset specific SDK at /ts_sdk_bldenv in container. It
	then creates docker image using image created in first step for the thing space SDK, image
	has all the necessary sdk files at /ts_sdk. In a final step it creates application
	docker image from the image created in second step which has application directory
	at /ts_sdk/apps or /ts_sdk/module_tests and other vendor dependent libraries
	like mbedtls at /ts_sdk/vendor. Script and eventually docker run command creates
	volume build/<app>/ to house all the compiled objects and binary images.

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

EOF
	exit 1
}

check_config()
{
	local flag="false"

	if [ -z "$CHIPSET_FAMILY" ]; then
		flag="true"
	fi
	if [ -z "$CHIPSET_MCU" ]; then
		flag="true"
	fi
	if [ -z "$DEV_BOARD" ]; then
		flag="true"
	fi
	if [ -z "$MODEM_TARGET" ]; then
		flag="true"
	fi
	if [ -z "$PROTOCOL" ]; then
		flag="true"
	fi
	if [ -z "$PROJ_ROOT" ]; then
		flag="true"
	fi

	if [ -z "$SDK_ROOT" ]; then
		flag="true"
	fi

	if [ -z "$SDK_APP_ROOT" ]; then
		flag="true"
	fi

	if [ $flag == "true" ]; then
		echo "Run source tools/scripts/config_build_env.sh help to setup environment"
		exit 1
	fi

}

check_for_file()
{
	if ! [ -f $1 ]; then
		echo "Specify dockerfile name along with the relative path"
		usage
	fi
}

cleanup_docker_images()
{
	for image in "$@"
	do
		echo "Cleaning image: $image"
		docker rmi -f $image
	done
}

cleanup_prev_docker_builds()
{
	if [ -d $MOUNT_DIR ]; then
		echo "Performing clean ups before docker build"
		rm -rf $MOUNT_DIR
	fi
}

check_build()
{
	if ! [ $? -eq 0 ]; then
		echo "Build failed"
		cleanup_docker_images "$@"
		exit 1
	fi
}

if ! [ -f $SCRIPT ]; then
	echo "Script must be run from the root of the repository"
	usage
fi

check_config

build_chipset_library()
{
	docker build -t $CHIPSET_IMAGE_NAME -f $1 $PROJ_ROOT
	docker run --rm -e BUILD_MCU=$CHIPSET_MCU -v $CHIPSET_BUILD_MOUNT:/build $CHIPSET_IMAGE_NAME
	check_build $CHIPSET_IMAGE_NAME
	cleanup_docker_images $CHIPSET_IMAGE_NAME
	echo "Chipset build success, generated static library lib$CHIPSET_MCU.a \
	at $MOUNT_DIR/$CHIPSET_FAMILY along with headers"
	exit 0

}

build_app()
{
	APP_DOCKER="$2"
	SDK_DOCKER="$1"
	shift
	shift
	APP_MAKE_ENV=""

	APP_NAME=""
	CHIPSET_BUILDENV=""

	for ARGUMENT in "$@"
	do
		KEY=$(echo $ARGUMENT | cut -f1 -d=)
		VALUE=$(echo $ARGUMENT | cut -f2 -d=)
		if ! [ -z "$VALUE" ]; then
			case "$KEY" in
				app_dir) BUILD_APP_ARG="-e APP=$VALUE"
					  APP_NAME=$VALUE
					  ;;
				chipset_env) CHIPSET_BUILDENV="-e CHIPSET_BUILDENV=$VALUE "
						;;
				*) APP_MAKE_ENV+="-e $KEY=$VALUE "
				;;
			esac
		fi
	done

	if [ -z "$APP_NAME" ]; then
		echo "Provide app_dir parameter"
		usage
	fi

	if [ -z "$CHIPSET_BUILDENV" ]; then
		echo "Provide chipset_env parameter"
		usage
	fi

	echo "Project root: $PROJ_ROOT"
	echo "SDK root: $SDK_ROOT"
	echo "Application directory to build: $APP_NAME"
	echo "Application specific build options: $APP_MAKE_ENV"

	docker build -t $TS_SDK_IMAGE_NAME -f $SDK_ROOT/$SDK_DOCKER $PROJ_ROOT
	docker build -t $APP_NAME -f $APP_DOCKER $PROJ_ROOT
	docker run $BUILD_APP_ARG $APP_MAKE_ENV -e PROTOCOL=$PROTOCOL \
		-e CHIPSET_FAMILY=$CHIPSET_FAMILY -e CHIPSET_MCU=$CHIPSET_MCU \
		-e DEV_BOARD=$DEV_BOARD -e MODEM_TARGET=$MODEM_TARGET $CHIPSET_BUILDENV \
		-v $MOUNT_DIR:/build $APP_NAME
	check_build $APP_NAME $TS_SDK_IMAGE_NAME
	cleanup_docker_images $APP_NAME $TS_SDK_IMAGE_NAME
	exit 0
}

install_sdk()
{
	if [ -z "$1" ]; then
		echo "Provide absolute path to install sdk"
		usage
	fi
	cp $SDK_ROOT $1
	if ! [ $? -eq 0 ]; then
		echo "Installing sdk failed..."
		exit 1
	fi
}

if [ "$#" -eq "0" ]; then
	echo "Insufficient arguments"
	usage
fi

if [ -z "$1" ]; then
	usage
elif [ $1 == "chipset" ]; then
	if [ -z "$2" ]; then
		echo "Specify relative path including dockerfile name.."
		usage
	fi
	shift
	check_for_file "$1"
	build_chipset_library "$@"
elif [ $1 == "app" ]; then
	if [ -z "$2" ]; then
		echo "Specify relative path including dockerfile name.."
		usage
	fi
	shift
	check_for_file "$SDK_ROOT/$1"
	check_for_file "$2"
	build_app "$@"
elif [ $1 == "install_sdk" ]; then
	shift
	install_sdk "$@"
else
	usage
fi
