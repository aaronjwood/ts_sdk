FROM ts_sdk
RUN mkdir -p build
COPY apps /ts_sdk/apps
COPY tools/config /ts_sdk/tools/config
COPY tools/scripts/config_build_env.sh /ts_sdk/tools/scripts/
RUN chmod a+x /ts_sdk/tools/scripts/config_build_env.sh
CMD cd /ts_sdk && \
    . tools/scripts/config_build_env.sh \
    && echo "In docker $PROTOCOL, $DEV_BOARD" \
    && echo "In docker $MK_HELPER_PATH, $SDK_ROOT, $PROJ_ROOT" \
    && cd /ts_sdk/apps/$APP && make build \
    && mkdir -p /build/$APP \
    && cp -r ./build/* /build/$APP
