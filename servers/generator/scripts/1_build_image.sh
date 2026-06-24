#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

docker build -t ai_stream2_generator servers/generator
