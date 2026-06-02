#!/usr/bin/env bash
set -euo pipefail

A="$(cd "$(dirname "$0")/../../../attachments/DeepStream-Yolo" && pwd)"

docker rm -f ultralytics_export 2>/dev/null || true
docker run -d --name ultralytics_export -w /app ultralytics/ultralytics:latest-python-export sleep infinity
docker cp "${A}/." ultralytics_export:/app
docker exec ultralytics_export bash -ec 'cd /app && for f in *.zip; do unzip -o -q "$f"; done'
docker exec ultralytics_export pip config set global.index-url https://mirrors.tuna.tsinghua.edu.cn/pypi/web/simple
docker exec ultralytics_export pip install -q onnxscript
