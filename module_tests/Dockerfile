FROM stm32f429_buildenv
RUN mkdir -p ts_sdk
RUN mkdir -p build
COPY module_tests /ts_sdk/module_tests
COPY sample_apps /ts_sdk/sample_apps
COPY sdk /ts_sdk/sdk
COPY platform /ts_sdk/platform
COPY vendor /ts_sdk/vendor
