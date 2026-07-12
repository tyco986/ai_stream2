FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
ENV CUDA_VER=13.1
ENV KAFKA_BROKER=ai_stream2_kafka:9092
ENV KAFKA_TOPIC=deepstream-detections
ENV KAFKA_EVENT_TOPIC=deepstream-events
ENV KAFKA_COMMAND_TOPIC=deepstream-commands
ENV DS_PREVIEW_RTP_HOST=ai_stream2_mediamtx
ENV HOST=0.0.0.0
ENV PORT=8092
ENV LOG_ROOT=/root/logs/deepstream
ENV GST_PLUGIN_PATH=/opt/nvidia/deepstream/deepstream/lib/gst-plugins

WORKDIR /app

RUN pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        librdkafka-dev

COPY servers/deepstream/requirements.txt /tmp/requirements.txt
RUN pip install --break-system-packages \
    /opt/nvidia/deepstream/deepstream/service-maker/python/pyservicemaker*.whl \
    && pip install --break-system-packages -r /tmp/requirements.txt

COPY servers/deepstream /app
COPY attachments/DeepStream-Yolo /root/attachments/DeepStream-Yolo
COPY attachments/deepstream-sahi /root/attachments/deepstream-sahi

RUN bash -ec '\
set -euo pipefail; \
mkdir -p /root/src /app/libs; \
for f in /root/attachments/DeepStream-Yolo/*.zip /root/attachments/deepstream-sahi/*.zip; do \
  [[ -f "$f" ]] && unzip -o -q "$f" -d /root/src; \
done; \
make -C /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo clean; \
make -C /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo; \
cp /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo/libnvdsinfer_custom_impl_Yolo.so /app/libs/; \
make -C /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face clean; \
make -C /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face; \
cp /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face/libnvdsinfer_custom_impl_Yolo_face.so /app/libs/; \
make -C /root/src/DeepStream-Yolo-Pose-master/nvdsinfer_custom_impl_Yolo_pose clean; \
make -C /root/src/DeepStream-Yolo-Pose-master/nvdsinfer_custom_impl_Yolo_pose; \
cp /root/src/DeepStream-Yolo-Pose-master/nvdsinfer_custom_impl_Yolo_pose/libnvdsinfer_custom_impl_Yolo_pose.so /app/libs/; \
SM=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader -i 0 2>/dev/null | head -1 | tr -d "." || echo "89"); \
export SM; \
make -C /root/src/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg clean; \
make -C /root/src/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg; \
cp /root/src/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg/libnvdsinfer_custom_impl_Yolo_seg.so /app/libs/; \
cd /opt/nvidia/deepstream/deepstream-9.0/sources/apps/sample_apps/deepstream-3d-action-recognition && make && make install; \
cp /opt/nvidia/deepstream/deepstream-9.0/lib/libnvds_custom_sequence_preprocess.so /app/libs/; \
bash /root/src/deepstream-sahi-master/install.sh --plugins-only; \
test -f /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_sahipreprocess.so; \
test -f /opt/nvidia/deepstream/deepstream/lib/gst-plugins/libnvdsgst_sahipostprocess.so; \
ls -la /app/libs/ \
'

RUN mkdir -p /root/logs/deepstream \
    && rm -rf /var/lib/apt/lists/* /root/.cache/gstreamer-1.0

EXPOSE 8092
CMD ["python3", "main.py"]
