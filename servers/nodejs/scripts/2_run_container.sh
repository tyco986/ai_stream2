#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
mkdir -p "${ROOT}/logs/nodejs"

docker network create ai_stream2_default 2>/dev/null || true
docker rm -f ai_stream2_nodejs 2>/dev/null || true

docker run -d \
  --name ai_stream2_nodejs \
  --network ai_stream2_default \
  -p 5173:5173 \
  -e HOST=0.0.0.0 \
  -e PORT=5173 \
  -v "${ROOT}/servers/nodejs:/app" \
  -v ai_stream2_nodejs_modules:/app/node_modules \
  -v "${ROOT}/logs/nodejs:/root/logs/nodejs" \
  ai_stream2_nodejs \
  sh -c 'npm run dev -- --host "$HOST" --port "$PORT"'

echo "Debug UI: http://127.0.0.1:5173/"
