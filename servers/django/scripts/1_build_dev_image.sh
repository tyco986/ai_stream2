#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

if [[ ! -f "${ROOT}/servers/django/keys/dev.pub" || ! -f "${ROOT}/servers/django/keys/ticket.pub" ]]; then
  echo "Missing vendor public keys. Run: bash servers/django/scripts/0_generate_vendor_keys.sh" >&2
  exit 1
fi

docker build \
  --target dev \
  -f servers/django/Dockerfile \
  -t "${PROJECT_NAME}_django_dev" \
  .
