#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

API_URL="http://127.0.0.1:8080"
ENDPOINT="${API_URL}/${PROJECT_NAME}/ffmpeg/rtsp/batch/probe"

usage() {
  cat <<EOF
usage: $0 --rtsp URL [--rtsp URL ...]

Batch probe RTSP streams with ffprobe.

Options:
  --rtsp URL           RTSP URL (repeatable, at least one)

Prerequisites: 1_build_dev_image.sh, 2_run_dev_container.sh
EOF
}

RTSPS=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --rtsp)
      RTSPS+=("$2")
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "unknown option: $1" >&2
      usage
      exit 1
      ;;
  esac
done

[[ ${#RTSPS[@]} -gt 0 ]] || { echo "at least one --rtsp is required" >&2; usage; exit 1; }

JSON_ARRAY="$(printf '%s\n' "${RTSPS[@]}" | python3 -c 'import json,sys; print(json.dumps([l.strip() for l in sys.stdin if l.strip()]))')"

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS -w "%{http_code}" -o "${RESPONSE_BODY}" \
  -X POST "${ENDPOINT}" \
  -H "Content-Type: application/json" \
  -d "{\"rtsps\":${JSON_ARRAY}}")"

cat "${RESPONSE_BODY}"
[[ "${HTTP_CODE}" == "200" ]] || exit 1
