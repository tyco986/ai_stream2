#!/usr/bin/env bash
# Run from project root. Production image: dev stage + baked application code.
set -euo pipefail

docker build \
  --target prod \
  -f servers/deepstream/Dockerfile \
  -t ai_stream2_deepstream_prod \
  .
