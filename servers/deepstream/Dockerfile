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

WORKDIR /app

RUN pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        librdkafka-dev \
        patch \
    && rm -rf /var/lib/apt/lists/*

COPY servers/deepstream/requirements.txt /tmp/requirements.txt
RUN pip install --break-system-packages \
    /opt/nvidia/deepstream/deepstream/service-maker/python/pyservicemaker*.whl \
    && pip install --break-system-packages -r /tmp/requirements.txt

COPY servers/deepstream/patches /app/patches
COPY attachments/DeepStream-Yolo /root/attachments/DeepStream-Yolo
COPY attachments/deepstream-sahi /root/attachments/deepstream-sahi

RUN bash -ec '\
set -euo pipefail; \
cd /opt/nvidia/deepstream/deepstream-9.0 && patch -p0 < /app/patches/gstnvinfer-interval-tensor-input.patch; \
make -C /opt/nvidia/deepstream/deepstream-9.0/sources/gst-plugins/gst-nvinfer CUDA_VER=${CUDA_VER} -j"$(nproc)"; \
make -C /opt/nvidia/deepstream/deepstream-9.0/sources/gst-plugins/gst-nvinfer CUDA_VER=${CUDA_VER} install; \
mkdir -p /root/src; \
for f in /root/attachments/DeepStream-Yolo/*.zip /root/attachments/deepstream-sahi/*.zip; do \
  [[ -f "$f" ]] && unzip -o -q "$f" -d /root/src; \
done; \
cd /root/src && patch -p0 < /app/patches/deepstream-yolo-rotation-angle-init.patch; \
bash /root/src/deepstream-sahi-master/install.sh --plugins-only; \
test -f /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_sahipreprocess.so; \
test -f /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_sahipostprocess.so; \
'

FROM base AS dev

ENV LD_PRELOAD=/opt/ai_stream2/servers/deepstream/libs/libnvdsinfer_custom_impl_Yolo_seg.so

RUN bash -ec '\
set -euo pipefail; \
mkdir -p "${DEEPSTREAM_LIBS_ROOT}"; \
make -C /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo clean; \
make -C /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo; \
cp /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo/libnvdsinfer_custom_impl_Yolo.so "${DEEPSTREAM_LIBS_ROOT}/"; \
make -C /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face clean; \
make -C /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face; \
cp /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face/libnvdsinfer_custom_impl_Yolo_face.so "${DEEPSTREAM_LIBS_ROOT}/"; \
SM=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader -i 0 2>/dev/null | head -1 | tr -d "." || echo "89"); \
export SM; \
make -C /root/src/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg clean; \
make -C /root/src/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg; \
cp /root/src/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg/libnvdsinfer_custom_impl_Yolo_seg.so "${DEEPSTREAM_LIBS_ROOT}/"; \
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
