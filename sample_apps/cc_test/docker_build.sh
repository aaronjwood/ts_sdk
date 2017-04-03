#!/bin/sh
# Copyright(C) 2016,2017 Verizon. All rights reserved
TOOLS_ROOT="$PWD/../../tools/docker/"
docker build -t stm32f429_buildenv -f $TOOLS_ROOT/Dockerfile /Users/v659353/Documents/ott_project/ott_firmware/
docker build -t ott_firmware -f ./Dockerfile /Users/v659353/Documents/ott_project/ott_firmware/
docker run --rm -v $PWD/build:/build ott_firmware
