FROM stm32f429_buildenv
RUN mkdir -p ts_sdk
RUN mkdir -p build
COPY sample_apps /ts_sdk/sample_apps
COPY sdk /ts_sdk/sdk
COPY platform /ts_sdk/platform
COPY vendor /ts_sdk/vendor
CMD cd /ts_sdk/sample_apps/sensor_examples && make PROTOCOL=SMSNAS_PROTOCOL build \
    && mkdir -p /build/sensor_examples \
    && cp -r ./build/* /build/sensor_examples
