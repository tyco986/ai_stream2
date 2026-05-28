#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

IMG="${FFMPEG_IMAGE:-ai_stream2_ffmpeg}"

docker build -t "${IMG}" "${ROOT}/ffmpeg"

cat <<EOF
${IMG} built from ${ROOT}/ffmpeg
EOF
