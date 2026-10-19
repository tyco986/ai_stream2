#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

PROJECT_NAME=ai_stream2

docker build \
  -f servers/frontend/pure-admin-thin-i18n/Dockerfile \
  -t "${PROJECT_NAME}_frontend_dev" \
  servers/frontend/pure-admin-thin-i18n
