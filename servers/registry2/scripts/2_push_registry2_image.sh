#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "usage: $0 <local-image> [name:tag]" >&2
  exit 1
fi

LOCAL="$1"
REMOTE="${2:-${LOCAL##*/}}"
TARGET="127.0.0.1:5000/${REMOTE}"

docker tag "${LOCAL}" "${TARGET}"
docker push "${TARGET}"
