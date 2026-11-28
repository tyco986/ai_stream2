#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"

docker compose -f "${ROOT}/docker-compose.yml" up -d registry2

echo "Registry: http://127.0.0.1:5000/v2/"
