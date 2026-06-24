#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

docker build -f servers/exporttrt/Dockerfile -t ai_stream2_exporttrt .
