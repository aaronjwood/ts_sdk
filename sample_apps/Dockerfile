FROM ts_sdk
ENV APP=cc_test
ENV PROTO=SMSNAS_PROTOCOL
RUN mkdir -p build
COPY sample_apps /ts_sdk/sample_apps
COPY vendor /ts_sdk/vendor
COPY tools/config /ts_sdk/tools/config
CMD cd /ts_sdk/sample_apps/$APP && make PROTOCOL=$PROTO build \
    && mkdir -p /build/$APP \
    && cp -r ./build/* /build/$APP
