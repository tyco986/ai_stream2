#!/usr/bin/env bash
# Run from project root. Builds the dev image, then the prod image from the same Dockerfile.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME=ai_stream2

docker build \
  --target dev \
  -f "${ROOT}/servers/backend/Dockerfile" \
  -t "${PROJECT_NAME}_backend_dev" \
  "${ROOT}"

docker build \
  --target prod \
  -f "${ROOT}/servers/backend/Dockerfile" \
  -t "${PROJECT_NAME}_backend" \
  "${ROOT}"
