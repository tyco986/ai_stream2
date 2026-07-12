#!/usr/bin/env bash
set -euo pipefail

STREAMS="${1:-2}"
LIMIT="${2:-300}"

docker exec ai_stream2_deepstream bash -lc "
  cd /app
  echo '--- MAIN (tracker, batch probe) ---'
  python3 scripts/probe_path_benchmark.py main ${STREAMS} ${LIMIT}
  echo
  echo '--- BRANCH (nvvidconv per stream) ---'
  python3 scripts/probe_path_benchmark.py branch ${STREAMS} ${LIMIT}
"
