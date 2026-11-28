#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"

docker compose -f "${ROOT}/docker-compose-control-node.yml" up -d pgweb

echo "pgweb: http://127.0.0.1:8081"
