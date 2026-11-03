#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME=ai_stream2

docker build \
  --target dev \
  -f "${ROOT}/servers/backend/Dockerfile" \
  -t "${PROJECT_NAME}_backend_dev" \
  "${ROOT}"
