#!/bin/bash
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
Usage: 1) $SCRIPT_NAME chipset <docker file relative path including file>
       2) $SCRIPT_NAME app <relative path including file to sdk dockerfile> \
<relative path including file to app dockerfile> app_dir=<app directory name> chipset_env=<absolute path> \
[Optional application options in key=value pair]
       3) $SCRIPT_NAME install_sdk <absolute destination path>
       compiles and installs libsdk.a at the given path along with headers
       Note: Based on protocol selected it will also compile mbedtls and install
       compiled library at the given path. User needs to also provide APIs and
       other platform related variables similar to as found in <install path>/platform_inc
       headers to resolve missing dependent symbols.

Notes:

1) Optional application options will be provided as a environment variable to docker container
while building application.
2) Argument provided for chipset_env contains the absolute path inside container where
it finds related chipset header and libraries.
3) First build chipset before building any applications, option 1 in usage
4) Before running $SCRIPT_NAME, run tools/scripts/config_build_env.sh help to
setup necessary build environment.

Example Usage:

To build chipset sdk:
tools/scripts/build.sh chipset tools/docker/Dockerfile.stm32f4_dockerhub
Produces output at <repo>/build/$CHIPSET_FAMILY, later application can mount this
directory for its container.

To build Application:
tools/scripts/build.sh app tools/docker/Dockerfile.sdk_dockerhub \
tools/docker/Dockerfile.apps app_dir=sensor_examples chipset_env=/build/stm32f4 PROJ_NAME=bmp180
Where PROJ_NAME is application specific option which is optional.

To build Module tests (module_tests directory examples)
tools/scripts/build.sh app tools/docker/Dockerfile.sdk_dockerhub \
tools/docker/Dockerfile.module_tests app_dir=at_tcp_test chipset_env=/build/stm32f4

Description:
	Script to build various apps from apps and module_tests directories based on
	application dockerfile provided. It also support of building stm32f4 chipset.
	Script can be modified to append any other chipset. Chipset build artifacts
	can be found under ./build/$CHIPSET_FAMILY where application depends to
	get necessary headers and library. Currently, all the docker container
	uses ubuntu 16.04 base images with arm toolchian preinstalled at /ts_sdk_bldenv/toolchain
	and mounts ./build directory to output build results.
	Provided dockerfiles can be found at ./tools/docker/

	Script also installs sdk only components to destination, see option 3 in
	usage above.
EOF
	exit 1
}

check_config()
{
	local flag="false"

	if [ -z "$PROJ_ROOT" ]; then
		flag="true"
	fi

	if [ -z "$SDK_ROOT" ]; then
		flag="true"
	fi

	if [ -z "$SDK_APP_ROOT" ]; then
		flag="true"
	fi

	if [ $flag = "true" ]; then
		echo "Run tools/scripts/config_build_env.sh help to setup environment"
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

if [ "$1" = "help" ] || [ -z "$1" ]; then
	usage
fi

check_config

build_chipset_library()
{
	docker build -t $CHIPSET_IMAGE_NAME -f $1 $PROJ_ROOT
	docker run --rm -e CHIPSET_MCU=$CHIPSET_MCU -v $CHIPSET_BUILD_MOUNT:/build $CHIPSET_IMAGE_NAME
	check_build $CHIPSET_IMAGE_NAME
	cleanup_docker_images $CHIPSET_IMAGE_NAME
	echo "Chipset build success, generated static library lib$CHIPSET_MCU.a \
	at $MOUNT_DIR/$CHIPSET_FAMILY along with headers"
	exit 0

}

build_app()
{
	SDK_DOCKER="$1"
	APP_DOCKER="$2"
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

	docker build -t $TS_SDK_IMAGE_NAME -f $SDK_DOCKER $PROJ_ROOT
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
	cd $SDK_ROOT && make INSTALL_PATH=$1 clean && make INSTALL_PATH=$1 all && make clean
	if ! [ $? -eq 0 ]; then
		echo "Installing sdk failed..."
		exit 1
	fi
}

if [ "$#" -eq "0" ]; then
	echo "Insufficient arguments"
	usage
fi

if [ "$1" = "chipset" ]; then
	if [ -z "$2" ]; then
		echo "Specify relative path including dockerfile name.."
		usage
	fi
	shift
	check_for_file "$1"
	build_chipset_library "$@"
elif [ "$1" = "app" ]; then
	if [ -z "$2" ]; then
		echo "Specify relative path including dockerfile name.."
		usage
	fi
	shift
	check_for_file "$1"
	check_for_file "$2"
	build_app "$@"
elif [ "$1" = "install_sdk" ]; then
	shift
	install_sdk "$@"
else
	usage
fi
