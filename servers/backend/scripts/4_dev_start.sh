#!/usr/bin/env bash
set -euo pipefail

cp /tmp/build/backend /app/backend
exec /app/backend
