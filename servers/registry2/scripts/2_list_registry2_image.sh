#!/usr/bin/env bash
# Run from project root. Print name:tag stored in the registry.
set -euo pipefail

BASE="http://127.0.0.1:5000"
mapfile -t NAMES < <(curl -fsS "${BASE}/v2/_catalog" | jq -r '.repositories // [] | .[]')
LINES=""
for NAME in "${NAMES[@]}"; do
  TAGS="$(curl -fsS "${BASE}/v2/${NAME}/tags/list" | jq -r --arg name "${NAME}" '.tags // [] | .[] | "\($name):\(.)"')"
  LINES="${LINES}${TAGS}"$'\n'
done
LINES="${LINES%$'\n'}"
echo "${LINES:-(empty)}"
