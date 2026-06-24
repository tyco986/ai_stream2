#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
API_URL="http://127.0.0.1:8092"
ENDPOINT="${API_URL}/ai_stream2/deepstream/build_pipeline"

usage() {
  cat <<EOF
usage: $0 --input path/to/config_dir --name NAME

Load generator config dir and attach task-specific drawer via DeepStream API.

Options:
  --input PATH   Config directory under configs/ (required)
  --name NAME    Pipeline instance name (required)

Prerequisites: 1_build_image.sh, 2_run_container.sh
EOF
}

INPUT=""
NAME=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --input)
      INPUT="$2"
      shift 2
      ;;
    --name)
      NAME="$2"
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
[[ -n "$NAME" ]] || { echo "--name is required" >&2; usage; exit 1; }

INPUT="$(realpath "$INPUT")"
[[ -d "$INPUT" ]] || { echo "config dir not found: $INPUT" >&2; exit 1; }
[[ -f "$INPUT/pipeline.yml" ]] || { echo "pipeline.yml not found in $INPUT" >&2; exit 1; }

REL="${INPUT#${ROOT}/configs/}"
if [[ "$REL" == "$INPUT" ]]; then
  echo "input must be under ${ROOT}/configs/" >&2
  exit 1
fi
CONTAINER_INPUT="/root/configs/${REL}"

RESPONSE_BODY="$(mktemp)"
trap 'rm -f "${RESPONSE_BODY}"' EXIT

HTTP_CODE="$(curl -sS -w "%{http_code}" -o "${RESPONSE_BODY}" \
  -X POST "${ENDPOINT}" \
  -H "Content-Type: application/json" \
  -d "{\"input\":\"${CONTAINER_INPUT}\",\"name\":\"${NAME}\"}")"

cat "${RESPONSE_BODY}"
echo

[[ "${HTTP_CODE}" == "200" ]] || exit 1
