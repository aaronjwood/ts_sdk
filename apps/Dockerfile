FROM ts_sdk
ENV APP=cc_test
ENV PROTO=SMSNAS_PROTOCOL
RUN mkdir -p build
COPY apps /ts_sdk/apps
COPY tools/config /ts_sdk/tools/config
CMD cd /ts_sdk/apps/$APP && make PROTOCOL=$PROTO build \
    && mkdir -p /build/$APP \
    && cp -r ./build/* /build/$APP
