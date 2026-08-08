#!/usr/bin/env bash
# Run from project root. Production image: deps + baked application code.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

docker build \
  --target prod \
  -f servers/ffmpeg/Dockerfile \
  -t "${PROJECT_NAME}_ffmpeg_prod" \
  .
