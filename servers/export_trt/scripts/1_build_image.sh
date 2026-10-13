#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

PROJECT_NAME=ai_stream2

docker build -f servers/export_trt/Dockerfile -t "${PROJECT_NAME}_export_trt" servers/export_trt
