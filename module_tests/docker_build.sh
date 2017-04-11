#!/bin/sh
# Copyright(C) 2017 Verizon. All rights reserved
TOOLS_ROOT="$PWD/../tools/docker/"
PROJ_ROOT=$(dirname $PWD)
SCRIPT=`basename "$0"`
if ! [ -f $SCRIPT ]; then
	echo "Script must be run from app source directory"
	exit 1
fi
docker build -t stm32f429_buildenv -f $TOOLS_ROOT/Dockerfile $PROJ_ROOT
docker build -t ts_sdk -f ./Dockerfile $PROJ_ROOT
docker run --rm -v $PWD/build:/build ts_sdk
