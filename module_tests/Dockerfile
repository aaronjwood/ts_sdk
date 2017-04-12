FROM ts_sdk
ENV APP=at_sms_test
ENV PROTO=SMSNAS_PROTOCOL
RUN mkdir -p build
COPY module_tests /ts_sdk/module_tests
COPY tools/config /ts_sdk/tools/config
CMD cd /ts_sdk/module_tests/$APP && make PROTOCOL=$PROTO build \
    && mkdir -p /build/$APP \
    && cp -r ./build/* /build/$APP
