#!/usr/bin/env bash
set -euo pipefail

cmake -S /src -B /tmp/build -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/build -j"$(nproc)"
bash /src/scripts/5_install_artifacts.sh
exec /app/deepstream
