#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

docker build \
  --target dev \
  -f servers/generator/Dockerfile \
  -t "${PROJECT_NAME}_generator_dev" \
  .
