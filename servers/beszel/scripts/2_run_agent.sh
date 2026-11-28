#!/usr/bin/env bash
# Run from project root. Compose starts the agent; 5_agent_start.sh fetches KEY and TOKEN.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"

docker compose -f "${ROOT}/docker-compose.yml" up -d beszel_agent
