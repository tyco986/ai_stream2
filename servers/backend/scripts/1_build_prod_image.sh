#!/usr/bin/env bash
# Run from project root. Production image: deps + baked application code.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME=ai_stream2

docker build \
  --target prod \
  -f "${ROOT}/servers/backend/Dockerfile" \
  -t "${PROJECT_NAME}_backend_prod" \
  "${ROOT}"
