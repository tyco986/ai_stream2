#!/usr/bin/env bash
# Stops the local demo services started by start_local_demo.sh.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
PID_FILE="${PROJECT_ROOT}/deepstream/storage/.video2rtsp.pid"

cd "${PROJECT_ROOT}"

if ! command -v docker >/dev/null 2>&1; then
  echo "Missing command: docker" >&2
  exit 1
fi

echo "=> docker compose stop deepstream kafka ..."
docker compose stop deepstream kafka
rm -f "${PID_FILE}"

echo "Stopped deepstream and kafka."
