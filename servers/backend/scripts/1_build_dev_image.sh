#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

PROJECT_NAME=ai_stream2

docker build \
  --target dev \
  -f servers/backend/Dockerfile \
  -t "${PROJECT_NAME}_backend_dev" \
  .
