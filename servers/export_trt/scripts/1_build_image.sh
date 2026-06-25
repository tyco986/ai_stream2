#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

docker build -f servers/export_trt/Dockerfile -t ai_stream2_export_trt .
