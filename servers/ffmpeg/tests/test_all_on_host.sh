#!/usr/bin/env bash
# Run from project root. Executes tests inside ai_stream2_ffmpeg.
set -euo pipefail

CONTAINER="${FFMPEG_CONTAINER_NAME:-ai_stream2_ffmpeg}"

exec docker exec "${CONTAINER}" python3 /app/tests/test_all.py "$@"
