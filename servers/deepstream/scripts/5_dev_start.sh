#!/usr/bin/env bash
set -euo pipefail

cmake -S /src -B /src/build -DCMAKE_BUILD_TYPE=Release
cmake --build /src/build -j"$(nproc)"
bash /src/scripts/5_install_artifacts.sh /src/build
export GST_REGISTRY=/tmp/gst-registry.bin
exec /app/deepstream
