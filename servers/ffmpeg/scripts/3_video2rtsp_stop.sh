#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

API_URL="http://127.0.0.1:8080"
ENDPOINT="${API_URL}/${PROJECT_NAME}/ffmpeg/video2rtsp_stop"

usage() {
  cat <<EOF
usage: $0 --rtsp URL

Stop RTSP publisher(s). Use --rtsp all to stop all.

Options:
  --rtsp URL           RTSP URL or "all" (required)

Prerequisites: 1_build_image.sh, 2_run_container.sh
EOF
}

RTSP=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --rtsp)
      RTSP="$2"
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

[[ -n "$RTSP" ]] || { echo "--rtsp is required" >&2; usage; exit 1; }

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS -w "%{http_code}" -o "${RESPONSE_BODY}" \
  -X POST "${ENDPOINT}" \
  -H "Content-Type: application/json" \
  -d "{\"rtsp\":\"${RTSP}\"}")"

if [[ "${HTTP_CODE}" == "200" ]]; then
  cat "${RESPONSE_BODY}"
else
  cat "${RESPONSE_BODY}"
  exit 1
fi
