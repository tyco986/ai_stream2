#!/usr/bin/env bash
set -euo pipefail

cp /tmp/build/backend /usr/local/bin/backend
exec backend
