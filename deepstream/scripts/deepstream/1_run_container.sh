#!/usr/bin/env bash
set -euo pipefail

docker network create ai_stream2_default 2>/dev/null || true
docker rm -f ai_stream2_deepstream ai_stream2-deepstream-1 deepstream 2>/dev/null || true

docker run -d \
  --name ai_stream2_deepstream \
  --network ai_stream2_default \
  --gpus all \
  --dns 8.8.8.8 \
  --dns 114.114.114.114 \
  -p 9000:9000 \
  -p 2222:22 \
  -e KAFKA_BROKER=ai_stream2_kafka:9092 \
  -e KAFKA_TOPIC=deepstream-detections \
  -e KAFKA_EVENT_TOPIC=deepstream-events \
  -e KAFKA_COMMAND_TOPIC=deepstream-commands \
  -e DS_PREVIEW_RTP_HOST=ai_stream2_mediamtx \
  -v /home/admin/project/ai_stream2/deepstream:/app \
  -w /app \
  nvcr.io/nvidia/deepstream:9.0-triton-multiarch \
  sleep infinity

docker exec ai_stream2_deepstream bash -ec '
pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple
pip install --break-system-packages \
  /opt/nvidia/deepstream/deepstream/service-maker/python/pyservicemaker*.whl
pip install --break-system-packages pyyaml
pip install --break-system-packages av
'

mkdir -p /home/admin/project/ai_stream2/deepstream/libs
docker cp /home/admin/project/ai_stream2/deepstream/attachments/DeepStream-Yolo/. ai_stream2_deepstream:/tmp/DeepStream-Yolo/
docker exec ai_stream2_deepstream bash -ec 'cd /tmp/DeepStream-Yolo && for f in *.zip; do unzip -o -q "$f"; done'

docker exec ai_stream2_deepstream bash -ec '
set -euo pipefail
export CUDA_VER=13.1
mkdir -p /app/libs
make -C /tmp/DeepStream-Yolo/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo clean
make -C /tmp/DeepStream-Yolo/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo
cp /tmp/DeepStream-Yolo/DeepStream-Yolo-master/nvdsinfer_custom_impl_Yolo/libnvdsinfer_custom_impl_Yolo.so /app/libs/
make -C /tmp/DeepStream-Yolo/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face clean
make -C /tmp/DeepStream-Yolo/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face
cp /tmp/DeepStream-Yolo/DeepStream-Yolo-Face-master/nvdsinfer_custom_impl_Yolo_face/libnvdsinfer_custom_impl_Yolo_face.so /app/libs/
make -C /tmp/DeepStream-Yolo/DeepStream-Yolo-Pose-master/nvdsinfer_custom_impl_Yolo_pose clean
make -C /tmp/DeepStream-Yolo/DeepStream-Yolo-Pose-master/nvdsinfer_custom_impl_Yolo_pose
cp /tmp/DeepStream-Yolo/DeepStream-Yolo-Pose-master/nvdsinfer_custom_impl_Yolo_pose/libnvdsinfer_custom_impl_Yolo_pose.so /app/libs/
export SM=$(nvidia-smi --query-gpu=compute_cap --format=csv,noheader -i 0 | head -1 | tr -d ".")
[[ -n "${SM}" ]] || { echo "nvidia-smi: cannot detect GPU SM" >&2; exit 1; }
make -C /tmp/DeepStream-Yolo/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg clean
make -C /tmp/DeepStream-Yolo/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg
cp /tmp/DeepStream-Yolo/DeepStream-Yolo-Seg-master/nvdsinfer_custom_impl_Yolo_seg/libnvdsinfer_custom_impl_Yolo_seg.so /app/libs/
cd /opt/nvidia/deepstream/deepstream-9.0/sources/apps/sample_apps/deepstream-3d-action-recognition && make && make install
cp /opt/nvidia/deepstream/deepstream-9.0/lib/libnvds_custom_sequence_preprocess.so /app/libs/
ls -la /app/libs/
'

docker exec ai_stream2_deepstream bash /app/scripts/deepstream/ssh.sh

cat <<'EOF'
ports:
  9000  DeepStream REST API (app)
  2222  SSH (root, password set by ssh.sh)
EOF
