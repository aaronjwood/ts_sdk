FROM stm32f429_buildenv
RUN mkdir -p ts_sdk
RUN mkdir -p build
COPY sample_apps /ts_sdk/sample_apps
COPY sdk /ts_sdk/sdk
COPY platform /ts_sdk/platform
COPY vendor /ts_sdk/vendor
CMD cd /ts_sdk/sample_apps/cc_test && make PROTOCOL=SMSNAS_PROTOCOL build \
    && mkdir -p /build/cc_test \
    && cp -r ./build/* /build/cc_test
