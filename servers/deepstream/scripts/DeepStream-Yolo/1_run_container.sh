#!/usr/bin/env bash
# Run from project root (or invoke directly; ROOT is resolved from this script).
set -euo pipefail

CONTAINER=DeepStream-Yolo
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
ATTACHMENTS="${ROOT}/attachments/DeepStream-Yolo"

docker rm -f "${CONTAINER}" 2>/dev/null || true
docker run -d --name "${CONTAINER}" -w /app ultralytics/ultralytics:latest sleep infinity
docker cp "${ATTACHMENTS}/." "${CONTAINER}:/app"
docker exec "${CONTAINER}" bash -ec 'cd /app && for f in *.zip; do unzip -o -q "$f"; done'
docker exec "${CONTAINER}" pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple
docker exec "${CONTAINER}" pip install -q onnxscript
