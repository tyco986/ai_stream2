#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

docker build \
  --target dev \
  -f servers/deepstream/Dockerfile \
  -t ai_stream2_deepstream_dev \
  .
