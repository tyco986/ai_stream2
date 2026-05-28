#!/usr/bin/env bash
set -euo pipefail

CONTAINER="${FFMPEG_CONTAINER_NAME:-ai_stream2_ffmpeg}"

exec docker exec "${CONTAINER}" python3 /app/tests/test_all.py "$@"
