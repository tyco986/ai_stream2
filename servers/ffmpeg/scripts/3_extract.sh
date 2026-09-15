#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

API_URL="http://127.0.0.1:8080"
ENDPOINT="${API_URL}/${PROJECT_NAME}/ffmpeg/video/extract"

usage() {
  cat <<EOF
usage: $0 --input path/to/video [--interval N]

Extract frames via FFmpeg API. interval=1 means every frame.
PNG files go to outputs/ffmpeg/extract/{basename}/.

Options:
  --input PATH         Video file (required)
  --interval N         Keep every Nth frame (default 1)

Prerequisites: 1_build_dev_image.sh, 2_run_dev_container.sh
EOF
}

INPUT=""
INTERVAL="1"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input)
      INPUT="$2"
      shift 2
      ;;
    --interval)
      INTERVAL="$2"
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

[[ -n "$INPUT" ]] || { echo "--input is required" >&2; usage; exit 1; }
[[ -f "$INPUT" ]] || { echo "input not found: $INPUT" >&2; exit 1; }

HTTP_CODE="$(curl -sS -w "%{http_code}" -o /tmp/ffmpeg_extract_response.json \
  -X POST "${ENDPOINT}" \
  -F "input=@${INPUT}" \
  -F "interval=${INTERVAL}")"

cat /tmp/ffmpeg_extract_response.json
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
