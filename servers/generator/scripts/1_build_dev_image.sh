#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

docker build \
  --target dev \
  -f servers/generator/Dockerfile \
  -t ai_stream2_generator_dev \
  .
