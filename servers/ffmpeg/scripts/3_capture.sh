#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"
API_URL="http://127.0.0.1:8080"
ENDPOINT="${API_URL}/${PROJECT_NAME}/ffmpeg/video/capture"

usage() {
  cat <<EOF
usage: $0 --input path/to/video --timestamp TS

Capture one frame via FFmpeg API. PNG is written to outputs/ffmpeg/capture/.

Options:
  --input PATH         Video file under recordings/ (required)
  --timestamp TS       HH:MM:SS or HH:MM:SS.mmm (required)

Prerequisites: 1_build_dev_image.sh, 2_run_dev_container.sh
EOF
}

INPUT=""
TIMESTAMP=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input)
      INPUT="$2"
      shift 2
      ;;
    --timestamp)
      TIMESTAMP="$2"
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
[[ -n "$TIMESTAMP" ]] || { echo "--timestamp is required" >&2; usage; exit 1; }

INPUT="$(realpath "$INPUT")"
[[ -f "$INPUT" ]] || { echo "input not found: $INPUT" >&2; exit 1; }

REL="${INPUT#${ROOT}/recordings/}"
if [[ "$REL" == "$INPUT" ]]; then
  echo "input must be under ${ROOT}/recordings/" >&2
  exit 1
fi
CONTAINER_INPUT="/root/recordings/${REL}"

HTTP_CODE="$(curl -sS -w "%{http_code}" -o /tmp/ffmpeg_capture_response.json \
  -X POST "${ENDPOINT}" \
  -F "input=${CONTAINER_INPUT}" \
  -F "timestamp=${TIMESTAMP}")"

cat /tmp/ffmpeg_capture_response.json
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
