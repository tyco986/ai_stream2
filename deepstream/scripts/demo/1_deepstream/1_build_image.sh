#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../../../" && pwd)"

docker build --no-cache \
  -t ai_stream2_deepstream \
  -f "${ROOT}/deepstream/Dockerfile" \
  "${ROOT}/deepstream"