FROM nvcr.io/nvidia/deepstream:9.0-triton-multiarch

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHONDONTWRITEBYTECODE=1
ENV PYTHONUNBUFFERED=1
ENV CUDA_VER=13.1
ENV HOST=0.0.0.0
ENV PORT=9000
ENV LOG_ROOT=/root/logs/export_trt

WORKDIR /app

RUN pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple

COPY servers/export_trt/requirements.txt /tmp/requirements.txt
RUN pip install --break-system-packages --no-cache-dir -r /tmp/requirements.txt

COPY servers/export_trt/main.py .
COPY servers/export_trt/utils ./utils
COPY attachments/DeepStream-Yolo /root/attachments/DeepStream-Yolo

RUN bash -ec '\
set -euo pipefail; \
mkdir -p /root/src /app/libs; \
for f in /root/attachments/DeepStream-Yolo/*.zip; do \
  [[ -f "$f" ]] && unzip -o -q "$f" -d /root/src; \
done; \
make -C /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo clean; \
make -C /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo; \
cp /root/src/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo/libnvdsinfer_custom_impl_Yolo.so /app/libs/; \
make -C /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face clean; \
make -C /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face; \
cp /root/src/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face/libnvdsinfer_custom_impl_Yolo_face.so /app/libs/; \
SM=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader -i 0 2>/dev/null | head -1 | tr -d "." || echo "89"); \
export SM; \
make -C /root/src/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg clean; \
make -C /root/src/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg; \
cp /root/src/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg/libnvdsinfer_custom_impl_Yolo_seg.so /app/libs/; \
ls -la /app/libs/ \
'

RUN mkdir -p /root/models/onnx /root/models/trt /root/logs/export_trt

EXPOSE 9000
CMD ["python3", "main.py"]
