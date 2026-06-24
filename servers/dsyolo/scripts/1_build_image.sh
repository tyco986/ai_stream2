#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
docker build -t ai_stream2_dsyolo -f "${ROOT}/servers/dsyolo/Dockerfile" "${ROOT}"
