#!/usr/bin/env bash
# Run from project root (or invoke directly; build context is servers/ffmpeg).
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
FFMPEG_DIR="${ROOT}/servers/ffmpeg"

docker build -t ai_stream2_ffmpeg "${FFMPEG_DIR}"

echo "ai_stream2_ffmpeg built"
