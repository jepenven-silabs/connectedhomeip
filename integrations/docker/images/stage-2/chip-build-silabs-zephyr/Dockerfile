ARG VERSION=1
FROM ghcr.io/project-chip/chip-build:${VERSION} AS build
LABEL org.opencontainers.image.source=https://github.com/project-chip/connectedhomeip

WORKDIR /opt/silabs/

RUN set -x \
    && python3 -m venv venv \
    && . venv/bin/activate \
    && pip3 install west \
    && west init -m https://github.com/SiliconLabsSoftware/zephyr-silabs \
    && west update \
    && pip3 install -r zephyr/scripts/requirements.txt \
    && west blobs fetch \
    && mkdir -p /opt/silabs/sdk \
    && cd /opt/silabs/sdk \
    && wget https://github.com/zephyrproject-rtos/sdk-ng/releases/download/v0.17.2/zephyr-sdk-0.17.2_linux-x86_64_minimal.tar.xz \
    && tar xvf zephyr-sdk-0.17.2_linux-x86_64_minimal.tar.xz \
    && rm zephyr-sdk-0.17.2_linux-x86_64_minimal.tar.xz \
    && cd zephyr-sdk-0.17.2 \
    && ./setup.sh -t arm-zephyr-eabi \
    && export ZEPHYR_SDK_INSTALL_DIR=/opt/silabs/sdk \
    && west zephyr-export \
    && : # last line

FROM ghcr.io/project-chip/chip-build:${VERSION}

COPY --from=build /opt/silabs/ /opt/silabs/

ENV SILABS_ZEPHYR_SDK_PATH=/opt/silabs
ENV ZEPHYR_BASE=/opt/silabs/zephyr
ENV ZEPHYR_SDK_INSTALL_DIR=/opt/silabs/sdk
