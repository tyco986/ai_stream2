#!/usr/bin/env bash
# Compile this service only. Third-party packages stay in the image.
set -euo pipefail

cd /app

apt-get update
apt-get install -y --no-install-recommends gcc g++ patchelf python3-dev

python3 -m nuitka \
  --mode=accelerated \
  --remove-output \
  --output-filename=export_onnx \
  --follow-import-to=utils \
  --follow-import-to=modules \
  --nofollow-import-to=torch \
  --nofollow-import-to=torchvision \
  --nofollow-import-to=ultralytics \
  --nofollow-import-to=cv2 \
  --nofollow-import-to=onnx \
  --nofollow-import-to=onnxslim \
  --nofollow-import-to=numpy \
  --nofollow-import-to=yaml \
  --nofollow-import-to=fastapi \
  --nofollow-import-to=starlette \
  --nofollow-import-to=uvicorn \
  --nofollow-import-to=pydantic \
  --nofollow-import-to=pydantic_core \
  --nofollow-import-to=multipart \
  --nofollow-import-to=PIL \
  main.py

rm -rf main.py utils modules
pip uninstall -y nuitka
apt-get purge -y gcc g++ patchelf python3-dev
apt-get autoremove -y
rm -rf /var/lib/apt/lists/*
