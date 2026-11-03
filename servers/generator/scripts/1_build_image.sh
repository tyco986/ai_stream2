#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
PROJECT_NAME="ai_stream2"
CONTEXT="${ROOT}/servers/generator"

docker build -f "${CONTEXT}/Dockerfile" -t "${PROJECT_NAME}_generator" "${CONTEXT}"
