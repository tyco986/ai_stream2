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
ENV LATENCY_PROBE_SO=/opt/nvidia/deepstream/deepstream/service-maker/modules/liblatency_probe.so

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
        librdkafka-dev \
        patch \
        ripgrep \
    && rm -rf /var/lib/apt/lists/*

COPY servers/deepstream/requirements.txt /tmp/requirements.txt
RUN pip install --break-system-packages \
    /opt/nvidia/deepstream/deepstream/service-maker/python/pyservicemaker*.whl \
    && pip install --break-system-packages -r /tmp/requirements.txt

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
cd /root/src/deepstream-sahi-master && patch -p1 < /app/patches/gstnvsahipostprocess-latency-timestamp.patch; \
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

COPY servers/deepstream/modules/latency_probe /tmp/latency_probe
COPY servers/deepstream/modules/stgcnpp_preprocess /tmp/stgcnpp_preprocess
RUN cmake -S /tmp/latency_probe -B /tmp/latency_probe/build \
    && cmake --build /tmp/latency_probe/build -j"$(nproc)" \
    && cp /tmp/latency_probe/build/liblatency_probe.so \
        /opt/nvidia/deepstream/deepstream/service-maker/modules/ \
    && rm -rf /tmp/latency_probe \
    && cmake -S /tmp/stgcnpp_preprocess -B /tmp/stgcnpp_preprocess/build \
    && cmake --build /tmp/stgcnpp_preprocess/build -j"$(nproc)" \
    && mkdir -p "${DEEPSTREAM_LIBS_ROOT}" \
    && cp /tmp/stgcnpp_preprocess/build/libnvds_stgcnpp_preprocess.so \
        "${DEEPSTREAM_LIBS_ROOT}/" \
    && test -f "${DEEPSTREAM_LIBS_ROOT}/libnvds_stgcnpp_preprocess.so" \
    && rm -rf /tmp/stgcnpp_preprocess

ENV LD_PRELOAD=/opt/ai_stream2/servers/deepstream/libs/libnvdsinfer_custom_impl_Yolo_seg.so

FROM base AS dev

RUN bash -ec '\
set -euo pipefail; \
mkdir -p "${DEEPSTREAM_LIBS_ROOT}"; \
cd /opt/nvidia/deepstream/deepstream-9.0/sources/apps/sample_apps/deepstream-3d-action-recognition && make && make install; \
cp /opt/nvidia/deepstream/deepstream-9.0/lib/libnvds_custom_sequence_preprocess.so "${DEEPSTREAM_LIBS_ROOT}/"; \
ls -la "${DEEPSTREAM_LIBS_ROOT}/"; \
'

RUN mkdir -p /root/logs/deepstream \
    && rm -rf /root/.cache/gstreamer-1.0

EXPOSE 8092
CMD ["python3", "main.py"]

FROM dev AS prod

COPY servers/deepstream /app
WORKDIR /app

EXPOSE 8092
CMD ["python3", "main.py"]
