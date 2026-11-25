#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <name:tag>" >&2
  exit 1
fi

docker pull "127.0.0.1:5000/$1"
