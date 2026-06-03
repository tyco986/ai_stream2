#!/usr/bin/env bash
# Run from project root. Requires ai_stream2_deepstream container.
set -euo pipefail

CONFIG=""

usage() {
  cat <<EOF
Usage: $0 --config PATH

  --config PATH  pipeline YAML under deepstream/ (e.g. configs/pipeline.yml)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    --config)
      [[ $# -ge 2 ]] || { usage >&2; exit 1; }
      CONFIG="$2"
      shift 2
      ;;
    *) usage >&2; exit 1 ;;
  esac
done

[[ -n "${CONFIG}" ]] || { usage >&2; exit 1; }

exec docker exec -it ai_stream2_deepstream python3 /app/main.py --config "${CONFIG}"
