FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch AS base

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
ENV CUDA_VER=13.1
ENV OMP_NUM_THREADS=1
ENV OMP_WAIT_POLICY=PASSIVE
ENV KAFKA_TOPIC=deepstream-detections
ENV KAFKA_EVENT_TOPIC=deepstream-events
ENV KAFKA_COMMAND_TOPIC=deepstream-commands
ENV DS_PREVIEW_RTP_HOST=ai_stream2_mediamtx
ENV HOST=0.0.0.0
ENV PORT=8092
ENV LOG_ROOT=/root/logs/deepstream
ENV GST_PLUGIN_PATH=/opt/nvidia/deepstream/deepstream/lib/gst-plugins
ENV DEEPSTREAM_LIBS_ROOT=/opt/ai_stream2/servers/deepstream/libs
ENV NVDS_ENABLE_LATENCY_MEASUREMENT=1
ENV NVDS_ENABLE_COMPONENT_LATENCY_MEASUREMENT=1

WORKDIR /app

RUN pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple

RUN find /etc/apt -type f \( -name "*.list" -o -name "*.sources" \) -print0 \
    | xargs -0 sed -i \
      -e "s|http://archive.ubuntu.com/ubuntu|https://mirrors.tuna.tsinghua.edu.cn/ubuntu|g" \
      -e "s|https://archive.ubuntu.com/ubuntu|https://mirrors.tuna.tsinghua.edu.cn/ubuntu|g" \
      -e "s|http://security.ubuntu.com/ubuntu|https://mirrors.tuna.tsinghua.edu.cn/ubuntu|g" \
      -e "s|https://security.ubuntu.com/ubuntu|https://mirrors.tuna.tsinghua.edu.cn/ubuntu|g" \
    && apt-get update \
    && apt-get install -y --no-install-recommends \
        cmake \
        librdkafka-dev \
        libpng-dev \
        libyaml-cpp-dev \
        patch \
    && rm -rf /var/lib/apt/lists/*

COPY servers/deepstream/patches /app/patches
COPY attachments/deepstream-sahi /root/attachments/deepstream-sahi

RUN bash -ec '\
set -euo pipefail; \
cd /opt/nvidia/deepstream/deepstream-9.0 && patch -p0 < /app/patches/gstnvinfer-interval-tensor-input.patch; \
make -C /opt/nvidia/deepstream/deepstream-9.0/sources/gst-plugins/gst-nvinfer CUDA_VER=${CUDA_VER} -j"$(nproc)"; \
make -C /opt/nvidia/deepstream/deepstream-9.0/sources/gst-plugins/gst-nvinfer CUDA_VER=${CUDA_VER} install; \
mkdir -p /root/src; \
for f in /root/attachments/deepstream-sahi/*.zip; do \
  [[ -f "$f" ]] && unzip -o -q "$f" -d /root/src; \
done; \
cd /root/src/deepstream-sahi-master && \
  patch -p1 < /app/patches/gstnvsahipostprocess-latency-timestamp.patch && \
  patch -p1 < /app/patches/nvdsinfer-yolo-zero-rotation-angle.patch; \
bash /root/src/deepstream-sahi-master/install.sh --plugins-only; \
test -f /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_sahipreprocess.so; \
test -f /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_sahipostprocess.so; \
test -f /opt/nvidia/deepstream/deepstream/lib/libnvds_infer_yolo.so; \
SEG_SRC=/root/src/nvdsinfer_custom_impl_Yolo_seg; \
test -d "${SEG_SRC}"; \
SM=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader -i 0 2>/dev/null | head -1 | tr -d "." || echo "89"); \
export SM CUDA_VER; \
make -C "${SEG_SRC}" clean; \
make -C "${SEG_SRC}"; \
mkdir -p "${DEEPSTREAM_LIBS_ROOT}"; \
cp "${SEG_SRC}/libnvdsinfer_custom_impl_Yolo_seg.so" "${DEEPSTREAM_LIBS_ROOT}/"; \
'

COPY servers/deepstream/modules /tmp/modules
RUN bash -ec '\
set -euo pipefail; \
export PATH="/usr/local/cuda-${CUDA_VER}/bin:${PATH}"; \
stub="/usr/local/cuda-${CUDA_VER}/lib64/stubs"; \
ln -sf "${stub}/libcuda.so" "${stub}/libcuda.so.1"; \
export LIBRARY_PATH="${stub}${LIBRARY_PATH:+:${LIBRARY_PATH}}"; \
mkdir -p "${DEEPSTREAM_LIBS_ROOT}" "${GST_PLUGIN_PATH}"; \
mapfile -t cmake_lists < <(find /tmp/modules -name CMakeLists.txt | sort); \
test "${#cmake_lists[@]}" -gt 0; \
for cm in "${cmake_lists[@]}"; do \
  src="$(dirname "$cm")"; \
  cmake -S "$src" -B "$src/build"; \
  cmake --build "$src/build" -j"$(nproc)"; \
  shopt -s nullglob; \
  for so in "$src/build"/*.so; do \
    name="$(basename "$so")"; \
    dest="${DEEPSTREAM_LIBS_ROOT}"; \
    if [[ "$name" == libnvdsgst_* ]]; then dest="${GST_PLUGIN_PATH}"; fi; \
    cp "$so" "${dest}/"; \
    test -f "${dest}/${name}"; \
  done; \
  for bin in "$src/build"/*; do \
    if [[ -f "$bin" && -x "$bin" && "$bin" != *.so ]]; then \
      name="$(basename "$bin")"; \
      cp "$bin" "/usr/local/bin/${name}"; \
      test -x "/usr/local/bin/${name}"; \
    fi; \
  done; \
done; \
test -x /usr/local/bin/pipeline_runner; \
test -x /usr/local/bin/deepstream_api; \
test -f "${DEEPSTREAM_LIBS_ROOT}/libgsource_remove_guard.so"; \
rm -rf /tmp/modules; \
'

ENV LD_PRELOAD=/opt/ai_stream2/servers/deepstream/libs/libgsource_remove_guard.so:/opt/ai_stream2/servers/deepstream/libs/libnvdsinfer_custom_impl_Yolo_seg.so

FROM base AS dev

RUN bash -ec '\
set -euo pipefail; \
mkdir -p "${DEEPSTREAM_LIBS_ROOT}"; \
cd /opt/nvidia/deepstream/deepstream-9.0/sources/apps/sample_apps/deepstream-3d-action-recognition && make && make install; \
cp /opt/nvidia/deepstream/deepstream-9.0/lib/libnvds_custom_sequence_preprocess.so "${DEEPSTREAM_LIBS_ROOT}/"; \
'

RUN mkdir -p /root/logs/deepstream \
    && rm -rf /root/.cache/gstreamer-1.0

EXPOSE 8092
CMD ["deepstream_api"]

FROM dev AS prod

COPY servers/deepstream /app
WORKDIR /app

EXPOSE 8092
CMD ["deepstream_api"]
