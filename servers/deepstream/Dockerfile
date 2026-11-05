FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch AS base

RUN printf '%s\n' \
        'Types: deb' \
        'URIs: https://mirrors.tuna.tsinghua.edu.cn/ubuntu/' \
        'Suites: noble noble-updates noble-backports' \
        'Components: main universe restricted multiverse' \
        'Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg' \
        '' \
        'Types: deb' \
        'URIs: https://mirrors.tuna.tsinghua.edu.cn/ubuntu/' \
        'Suites: noble-security' \
        'Components: main universe restricted multiverse' \
        'Signed-By: /usr/share/keyrings/ubuntu-archive-keyring.gpg' \
        > /etc/apt/sources.list.d/ubuntu.sources

ENV GST_PLUGIN_PATH=/opt/nvidia/deepstream/deepstream/lib/gst-plugins


FROM base AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        g++ \
        cmake \
        pkg-config \
        nlohmann-json3-dev \
        libyaml-cpp-dev \
        libzip-dev \
        libspdlog-dev \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        make \
        patch \
        unzip \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*
COPY src/patches /patches
COPY attachments/deepstream-sahi-master.zip /tmp/deepstream-sahi-master.zip
COPY scripts/apply_patches.sh /tmp/apply_patches.sh
RUN bash /tmp/apply_patches.sh
COPY CMakeLists.txt main.cpp /src/
COPY src /src/src
RUN cmake -S /src -B /tmp/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/build -j"$(nproc)" \
    && mkdir -p /app \
    && cp /tmp/build/deepstream /app/deepstream \
    && cp /tmp/build/src/plugins/boxsnapshot/libgstboxsnapshot.so ${GST_PLUGIN_PATH}/


FROM base

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libyaml-cpp0.8 \
        libzip4t64 \
        libspdlog1.12 \
        libfmt9 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /app/deepstream ./deepstream
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libgstboxsnapshot.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_infer.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_sahipreprocess.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_sahipostprocess.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/libnvds_infer_yolo.so /opt/nvidia/deepstream/deepstream/lib/
EXPOSE 8094
CMD ["./deepstream"]
