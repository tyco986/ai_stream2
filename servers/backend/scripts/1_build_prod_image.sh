#!/usr/bin/env bash
# Run from project root. Production image: deps + baked application code.
set -euo pipefail

PROJECT_NAME=ai_stream2

docker build \
  --target prod \
  -f servers/backend/Dockerfile \
  -t "${PROJECT_NAME}_backend_prod" \
  .
