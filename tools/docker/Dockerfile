FROM ubuntu:16.04
RUN apt-get update \
    && apt-get install -y wget \
    && apt-get install -y make \
    && apt-get install -y openocd \
    && apt-get install -y bzip2 \
    && apt-get install -y gcc-multilib
RUN mkdir -p ts_sdk_bldenv/toolchain

RUN wget -O gcc-arm-none-eabi-5_4-2016q2-20160622-linux.tar.bz2 https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q2-update/+download/gcc-arm-none-eabi-5_4-2016q2-20160622-linux.tar.bz2 \
 && tar -C /ts_sdk_bldenv/toolchain/ -jxvf gcc-arm-none-eabi-5_4-2016q2-20160622-linux.tar.bz2 \
 && rm -f gcc-arm-none-eabi-5_4-2016q2-20160622-linux.tar.bz2

COPY targets/stmicro/chipset /ts_sdk_bldenv/
