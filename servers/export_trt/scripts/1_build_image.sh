#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"

docker build -f "${ROOT}/servers/export_trt/Dockerfile" -t "${PROJECT_NAME}_export_trt" "${ROOT}/servers/export_trt"
