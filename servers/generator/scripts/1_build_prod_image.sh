#!/usr/bin/env bash
# Run from project root. Production image: dev stage + baked application code.
set -euo pipefail

docker build \
  --target prod \
  -f servers/generator/Dockerfile \
  -t ai_stream2_generator_prod \
  .
