#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"
CONTEXT="${ROOT}/servers/export_trt"

cp "${ROOT}/attachments/deepstream-sahi/nvdsinfer_custom_impl_Yolo_seg.zip" "${CONTEXT}/nvdsinfer_custom_impl_Yolo_seg.zip"
trap 'rm -f "${CONTEXT}/nvdsinfer_custom_impl_Yolo_seg.zip"' EXIT
docker build -f "${CONTEXT}/Dockerfile" -t "${PROJECT_NAME}_export_trt" "${CONTEXT}"
