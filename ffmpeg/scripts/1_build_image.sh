#!/usr/bin/env bash
set -euo pipefail

docker build -t ai_stream2_ffmpeg "$(cd "$(dirname "$0")/.." && pwd)"

echo "ai_stream2_ffmpeg built"
