FROM ts_sdk
ARG APP=cc_test
ARG PROTO=SMSNAS_PROTOCOL
RUN mkdir -p build
COPY sample_apps /ts_sdk/sample_apps
COPY vendor /ts_sdk/vendor
CMD cd /ts_sdk/sample_apps/$APP && make PROTOCOL=$PROTO build \
    && mkdir -p /build/$APP \
    && cp -r ./build/* /build/$APP
