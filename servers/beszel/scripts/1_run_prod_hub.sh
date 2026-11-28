#!/usr/bin/env bash
# Run from project root. Compose starts Hub and the agent entrypoint.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"

docker compose -f "${ROOT}/docker-compose-control-node.yml" up -d beszel_hub beszel_agent

echo "Beszel UI: http://127.0.0.1:8093"
