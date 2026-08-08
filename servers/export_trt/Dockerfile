FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch AS base

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
ENV CUDA_VER=13.1
ENV HOST=0.0.0.0
ENV PORT=9000
ENV LOG_ROOT=/root/logs/export_trt
ENV EXPORT_TRT_LIBS_ROOT=/opt/ai_stream2/servers/export_trt/libs

WORKDIR /app

RUN pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple

COPY servers/export_trt/requirements.txt /tmp/requirements.txt
RUN pip install --break-system-packages --no-cache-dir -r /tmp/requirements.txt

COPY attachments/DeepStream-Yolo /root/attachments/DeepStream-Yolo
COPY attachments/deepstream-sahi/nvdsinfer_custom_impl_Yolo_seg.zip /root/attachments/deepstream-sahi/nvdsinfer_custom_impl_Yolo_seg.zip

RUN bash -ec '\
set -euo pipefail; \
mkdir -p /root/src "${EXPORT_TRT_LIBS_ROOT}"; \
for f in /root/attachments/DeepStream-Yolo/*.zip; do \
  [[ -f "$f" ]] && unzip -o -q "$f" -d /root/src; \
done; \
unzip -o -q /root/attachments/deepstream-sahi/nvdsinfer_custom_impl_Yolo_seg.zip -d /root/src; \
make -C /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo clean; \
make -C /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo; \
cp /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo/libnvdsinfer_custom_impl_Yolo.so "${EXPORT_TRT_LIBS_ROOT}/"; \
make -C /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face clean; \
make -C /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face; \
cp /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face/libnvdsinfer_custom_impl_Yolo_face.so "${EXPORT_TRT_LIBS_ROOT}/"; \
SM=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader -i 0 2>/dev/null | head -1 | tr -d "." || echo "89"); \
export SM; \
SEG_SRC=/root/src/nvdsinfer_custom_impl_Yolo_seg; \
make -C "${SEG_SRC}" clean; \
make -C "${SEG_SRC}"; \
cp "${SEG_SRC}/libnvdsinfer_custom_impl_Yolo_seg.so" "${EXPORT_TRT_LIBS_ROOT}/"; \
ls -la "${EXPORT_TRT_LIBS_ROOT}/" \
'


FROM base AS dev

RUN mkdir -p /root/models/onnx /root/models/trt /root/logs/export_trt

EXPOSE 9000
CMD ["python3", "main.py"]


FROM dev AS prod

COPY servers/export_trt /app
WORKDIR /app

EXPOSE 9000
CMD ["python3", "main.py"]
