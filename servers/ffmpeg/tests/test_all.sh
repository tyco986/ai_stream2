#!/usr/bin/env bash
# Run from project root (or any cwd; resolves tests dir from script location).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python3 "${SCRIPT_DIR}/test_all.py" "$@"
