#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
START="$(cd "$(dirname "$0")" && pwd)/3_start_local.sh"
CONFIGS="${ROOT}/history/generator"
HOST_PORT="${DEEPSTREAM_HOST_PORT:-8094}"
STATUS="http://127.0.0.1:${HOST_PORT}/status"

while IFS= read -r config; do
  "${START}" --config "${config}"
  while true; do
    body="$(curl -sS --connect-timeout 2 -H "Accept-Language: en" "${STATUS}")"
    [[ "${body}" == *'"running":false'* ]] && break
    sleep 1
  done
done < <(find "${CONFIGS}" -type f -name '*video*.zip' | sort)
