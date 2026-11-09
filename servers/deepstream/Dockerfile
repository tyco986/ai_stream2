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
ENV LD_PRELOAD=/opt/ai_stream2/servers/deepstream/libs/libnvdsinfer_custom_impl_Yolo_seg.so
ENV OMP_NUM_THREADS=1
ENV OMP_WAIT_POLICY=PASSIVE
ENV NVDS_ENABLE_LATENCY_MEASUREMENT=1
ENV NVDS_ENABLE_COMPONENT_LATENCY_MEASUREMENT=1


FROM base AS toolchain

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        g++ \
        cmake \
        pkg-config \
        nlohmann-json3-dev \
        libyaml-cpp-dev \
        libzip-dev \
        zipcmp \
        zipmerge \
        ziptool \
        libspdlog-dev \
        libpng-dev \
        libjpeg-turbo8-dev \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        make \
        patch \
        unzip \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*
COPY src/patches /patches
COPY attachments/deepstream-sahi-master.zip /tmp/deepstream-sahi-master.zip
COPY scripts/5_apply_patches.sh /tmp/5_apply_patches.sh
RUN bash /tmp/5_apply_patches.sh
COPY attachments/nvdsinfer_custom_impl_Yolo_seg.zip /tmp/nvdsinfer_custom_impl_Yolo_seg.zip
RUN unzip -o -q /tmp/nvdsinfer_custom_impl_Yolo_seg.zip -d /root/src \
    && make -C /root/src/nvdsinfer_custom_impl_Yolo_seg CUDA_VER=13.1 SM=120 -j"$(nproc)" \
    && mkdir -p /opt/ai_stream2/servers/deepstream/libs \
    && cp /root/src/nvdsinfer_custom_impl_Yolo_seg/libnvdsinfer_custom_impl_Yolo_seg.so /opt/ai_stream2/servers/deepstream/libs/


FROM toolchain AS dev

WORKDIR /src
EXPOSE 8094


FROM toolchain AS build

COPY CMakeLists.txt main.cpp /src/
COPY src /src/src
COPY scripts/5_install_artifacts.sh /tmp/5_install_artifacts.sh
RUN cmake -S /src -B /tmp/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build /tmp/build -j"$(nproc)" \
    && bash /tmp/5_install_artifacts.sh


FROM base AS prod

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libyaml-cpp0.8 \
        libzip4t64 \
        libspdlog1.12 \
        libpng16-16t64 \
        libjpeg-turbo8 \
        libfmt9 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=build /app/deepstream ./deepstream
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libgstboxsnapshot.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libgstfadedrawer.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libgstpresenceeventcoder.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libgstcapturer.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libgstdslogger.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libgststgcnppactionlabel.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libgstnvsahipostprocesspose.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_infer.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_sahipreprocess.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_sahipostprocess.so /opt/nvidia/deepstream/deepstream/lib/gst-plugins/
COPY --from=build /opt/nvidia/deepstream/deepstream/lib/libnvds_infer_yolo.so /opt/nvidia/deepstream/deepstream/lib/
COPY --from=build /opt/ai_stream2/servers/deepstream/libs /opt/ai_stream2/servers/deepstream/libs
EXPOSE 8094
CMD ["./deepstream"]
