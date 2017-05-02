FROM ts_sdk
ENV APP=basic_template
ENV PROTO=SMSNAS_PROTOCOL
RUN mkdir -p build
COPY user_apps /ts_sdk/user_apps
COPY tools/config /ts_sdk/tools/config
CMD cd /ts_sdk/user_apps/$APP && make PROTOCOL=$PROTO build \
    && mkdir -p /build/$APP \
    && cp -r ./build/* /build/$APP
