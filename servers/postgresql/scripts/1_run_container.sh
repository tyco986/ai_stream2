#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"

docker compose -f "${ROOT}/docker-compose.yml" up -d postgresql

echo "PostgreSQL: ai_stream2_postgresql port 5432"
