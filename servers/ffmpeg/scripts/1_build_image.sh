#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

PROJECT_NAME=ai_stream2

docker build -f servers/ffmpeg/Dockerfile -t "${PROJECT_NAME}_ffmpeg" servers/ffmpeg
