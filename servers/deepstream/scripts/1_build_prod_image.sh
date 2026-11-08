#!/usr/bin/env bash
# Run from project root. Production image: deps + baked application code.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"
CONTEXT="${ROOT}/servers/deepstream"

docker build \
  --target prod \
  -f "${CONTEXT}/Dockerfile" \
  -t "${PROJECT_NAME}_deepstream_prod" \
  "${CONTEXT}"
