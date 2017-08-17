#!/bin/bash
# Copyright(C) 2017 Verizon. All rights reserved

DOCKERFILE_BASEROOT="$PROJ_ROOT/tools/docker"

MOUNT_DIR="$PROJ_ROOT/build"
# default mount point for the chipset build environment to output build artifacts
CHIPSET_BUILD_MOUNT="$MOUNT_DIR"

BUILD_APP_ARG=""

SCRIPT_NAME=`basename "$0"`
SCRIPT="tools/scripts/$SCRIPT_NAME"

CHIPSET_IMAGE_NAME="${CHIPSET_MCU}_buildenv"
TS_SDK_IMAGE_NAME="ts_sdk"

# this is used when creating tssdk client library
TS_SDK_CLIENT_IMAGE_NAME="ts_sdk_client"

# raspberry pi 3 related docker base image
RASPI_BASE_IMG="pi3_base"
RPI_IMGID=$(docker images -q $RASPI_BASE_IMG)
RASPI_BASE_FILE="Dockerfile.pi3_base"

# St micro related docker base image
STM_BASE_IMG="stmicro_base"
STM_IMGID=$(docker images -q $STM_BASE_IMG)
STM_BASE_FILE="Dockerfile.stm_base"

EXIT_CODE=

if [ "$CHIPSET_FAMILY" = "stm32l4" ]; then
	CHIPSET_HAL_DIR="$PROJ_ROOT/targets/stmicro/chipset/stm32l4/chipset_hal"
	CHIPSET_ROOT="$PROJ_ROOT/targets/stmicro/chipset/stm32l4"
	CHIPSET_STM32L4_BRANCH="stm32l4_chipset"
fi

usage()
{
	cat << EOF
Usage: 1) $SCRIPT_NAME chipset <docker file relative path including file>
       2) $SCRIPT_NAME app <relative path including file to sdk dockerfile> \
<relative path including file to app dockerfile> app_dir=<app directory name> chipset_env=<absolute path> \
[Optional application options in key=value pair]
       3) $SCRIPT_NAME create_sdk_client <relative path with filename to docker file>
       compiles and installs libsdk.a at ./build directory with include and lib
       folders containing necessary headers and library libsdk.a

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
tools/scripts/build.sh chipset tools/docker/Dockerfile.chipset_stm32f4
Produces output at <repo>/build/$CHIPSET_FAMILY, later application can mount this
directory for its container and hence the chipset_env parameters should be /build/<CHIPSET_FAMILY>
when compiling application if it depends on the chipset build environment.

To build Application:
tools/scripts/build.sh app tools/docker/Dockerfile.sdk_stm32 \
tools/docker/Dockerfile.apps app_dir=sensor_examples chipset_env=/build/stm32f4 PROJ_NAME=bmp180
Where PROJ_NAME is application specific option which is optional, chipset_env may be
optional as well depending on the platform or devboard used.

To build Module tests (module_tests directory examples)
tools/scripts/build.sh app tools/docker/Dockerfile.sdk_stm32 \
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

error_exit()
{
	if [ "$2" = "true" ]; then
		if ! [ "$3" -eq 0 ]; then
			echo "$1" 1>&2
			exit 1
		fi
	else
		echo "$1" 1>&2
		exit 1
	fi

}

build_base_images()
{
	echo "Building docker image $IMAGE_NAME for the first time, this may take few minutes..#######"
	docker build -t $1 -f $DOCKERFILE_BASEROOT/$2 $DOCKERFILE_BASEROOT
	EXIT_CODE=$?
	error_exit "building docker base image $IMAGE_NAME failed" "true" $EXIT_CODE
}

check_docker_images_config()
{
	IMAGE_NAME=
	BASE_FILE=
	if [ "$DEV_BOARD" = "raspberry_pi3" ]; then
		if [ -z "$RPI_IMGID" ]; then
			IMAGE_NAME=$RASPI_BASE_IMG
			BASE_FILE=$RASPI_BASE_FILE
		fi
	elif [ "$DEV_BOARD" = "nucleo" ] || [ "$DEV_BOARD" = "beduin" ]; then
		if [ -z "$STM_IMGID" ]; then
			IMAGE_NAME=$STM_BASE_IMG
			BASE_FILE=$STM_BASE_FILE
		fi
	else
		error_exit "Select valid DEV_BOARD" "false"
	fi

	if ! [ -z "$IMAGE_NAME" ]; then
		build_base_images $IMAGE_NAME $BASE_FILE
	fi
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
		error_exit "Run tools/scripts/config_build_env.sh help to setup environment" "false"
	fi

}

check_for_file()
{
	if ! [ -f $1 ]; then
		echo "Specify dockerfile name along with the relative path"
		exit 1
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
check_docker_images_config

checkout_chipset_hal()
{
	if [ $CHIPSET_FAMILY = 'stm32l4' ]; then
		if [ ! -d $CHIPSET_HAL_DIR ]; then
			echo "Checking out chipset halfiles, takes approx 90 seconds..."
			BRANCH_NAME=$CHIPSET_STM32L4_BRANCH
			cd $CHIPSET_ROOT && git clone https://github.com/verizonlabs/chipset_hal.git
			EXIT_CODE=$?
			error_exit "chipset_hal images clone failed" "true" $EXIT_CODE

			echo "Checking out $BRANCH_NAME branch ..."
			cd chipset_hal && git checkout $BRANCH_NAME
			EXIT_CODE=$?
			error_exit "git checkout failed" "true" $EXIT_CODE

			tar -xjf STM32Cube_FW_L4_V1.8.0.tar.bz2
			EXIT_CODE=$?
			error_exit "Untarring of chipset hal image is failed" "true" $EXIT_CODE

			rm STM32Cube_FW_L4_V1.8.0.tar.bz2
			cd $PROJ_ROOT
		else
			echo "chipset_hal folder is already existing"
		fi
	fi
}

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
		exit 1
	fi

	echo "Project root: $PROJ_ROOT"
	echo "SDK root: $SDK_ROOT"
	echo "Application directory to build: $APP_NAME"
	echo "Application specific build options: $APP_MAKE_ENV"

	docker build -t $TS_SDK_IMAGE_NAME -f $SDK_DOCKER $PROJ_ROOT
	docker build -t $APP_NAME -f $APP_DOCKER $PROJ_ROOT
	docker run $BUILD_APP_ARG $APP_MAKE_ENV -e PROTOCOL=$PROTOCOL \
		-e CHIPSET_FAMILY=$CHIPSET_FAMILY -e CHIPSET_MCU=$CHIPSET_MCU \
		-e DEV_BOARD=$DEV_BOARD -e MODEM_TARGET=$MODEM_TARGET \
		-e GPS_CHIPSET=$GPS_CHIPSET $CHIPSET_BUILDENV \
		-v $MOUNT_DIR:/build $APP_NAME
	check_build $APP_NAME $TS_SDK_IMAGE_NAME
	cleanup_docker_images $APP_NAME $TS_SDK_IMAGE_NAME
	exit 0
}

create_sdk_client()
{
	if [ -z "$1" ]; then
		echo "Specify relative path including dockerfile name.."
		exit 1
	fi
	docker build -t $TS_SDK_CLIENT_IMAGE_NAME -f $1 $PROJ_ROOT
	docker run -e PROTOCOL=$PROTOCOL \
		-e CHIPSET_FAMILY=$CHIPSET_FAMILY -e CHIPSET_MCU=$CHIPSET_MCU \
		-e DEV_BOARD=$DEV_BOARD -e MODEM_TARGET=$MODEM_TARGET \
		-e GPS_CHIPSET=$GPS_CHIPSET $CHIPSET_BUILDENV \
		-v $MOUNT_DIR:/build/sdk_client $TS_SDK_CLIENT_IMAGE_NAME
	check_build $TS_SDK_CLIENT_IMAGE_NAME
	echo "Build was successful..."
	cleanup_docker_images $TS_SDK_CLIENT_IMAGE_NAME
	exit 0
}

if [ "$#" -eq "0" ]; then
	echo "Insufficient arguments"
	usage
fi

if [ "$1" = "chipset" ]; then
	if [ -z "$2" ]; then
		echo "Specify relative path including dockerfile name.."
		exit 1
	fi
	shift
	check_for_file "$1"
	checkout_chipset_hal "$@"
	build_chipset_library "$@"
elif [ "$1" = "app" ]; then
	if [ -z "$2" ]; then
		echo "Specify relative path including dockerfile name.."
		exit 1
	fi
	shift
	check_for_file "$1"
	check_for_file "$2"
	build_app "$@"
elif [ "$1" = "create_sdk_client" ]; then
	shift
	create_sdk_client "$@"
else
	usage
fi
