#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"

docker compose -f "${ROOT}/docker-compose-control-node.yml" up -d mediamtx

echo "MediaMTX: ai_stream2_mediamtx config=mediamtx_prod.yml ports 8554 8889 8189 9996 9997"
