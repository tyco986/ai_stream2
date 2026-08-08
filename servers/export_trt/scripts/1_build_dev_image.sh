#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

docker build \
  --target dev \
  -f servers/export_trt/Dockerfile \
  -t "${PROJECT_NAME}_export_trt_dev" \
  .
