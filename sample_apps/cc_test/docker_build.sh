#!/bin/sh
# Copyright(C) 2016,2017 Verizon. All rights reserved
TOOLS_ROOT="$PWD/../../tools/docker/"
PROJ_ROOT=$(dirname $(dirname $PWD))
SCRIPT=`basename "$0"`
if ! [ -f $SCRIPT ]; then
	echo "Script must be run app directory"
	exit 1
fi
docker build -t stm32f429_buildenv -f $TOOLS_ROOT/Dockerfile $PROJ_ROOT
docker build -t ott_firmware -f ./Dockerfile $PROJ_ROOT
docker run --rm -v $PWD/build:/build ott_firmware
