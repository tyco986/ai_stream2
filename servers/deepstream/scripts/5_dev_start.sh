#!/usr/bin/env bash
set -euo pipefail

bash "$(cd "$(dirname "$0")" && pwd)/5_install_artifacts.sh" /tmp/build
rm -f /tmp/gst-registry.bin
exec deepstream
