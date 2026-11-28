#!/usr/bin/env bash
# Run from project root. Builds the dev image, then the prod image from the same Dockerfile.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"
CONTEXT="${ROOT}/servers/deepstream"

docker build \
  --target dev \
  -f "${CONTEXT}/Dockerfile" \
  -t "${PROJECT_NAME}_deepstream_dev" \
  "${CONTEXT}"

docker build \
  --target prod \
  -f "${CONTEXT}/Dockerfile" \
  -t "${PROJECT_NAME}_deepstream" \
  "${CONTEXT}"
