#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

cd "${ROOT}"

docker build \
  --target prod \
  -t "${PROJECT_NAME}_nodejs_prod" \
  servers/nodejs
