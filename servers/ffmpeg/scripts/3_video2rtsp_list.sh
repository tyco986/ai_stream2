#!/usr/bin/env bash
# Run from project root. Requires ai_stream2_ffmpeg on :8080.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=_api_curl.sh
. "${SCRIPT_DIR}/_api_curl.sh"

usage() {
  cat <<EOF
Usage: $0 [--help]

  GET http://127.0.0.1:8080/ffmpeg/video2rtsp_list
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help) usage; exit 0 ;;
    *) usage >&2; exit 1 ;;
  esac
done

api_curl http://127.0.0.1:8080/ffmpeg/video2rtsp_list
echo
