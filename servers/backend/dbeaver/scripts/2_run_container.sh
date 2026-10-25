#!/usr/bin/env bash
set -euo pipefail

PROJECT_NAME=ai_stream2
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${PROJECT_NAME}_cloudbeaver" 2>/dev/null || true

docker run -d \
  --name "${PROJECT_NAME}_cloudbeaver" \
  --network "${PROJECT_NAME}_default" \
  -p 8978:8978 \
  -e CB_SERVER_NAME=ai_stream2 \
  -e CB_SERVER_URL=http://127.0.0.1:8978/ \
  -e CB_ADMIN_NAME=puskas \
  -e CB_ADMIN_PASSWORD=0000 \
  -v "${SCRIPT_DIR}/data-sources.json:/opt/cloudbeaver/workspace/GlobalConfiguration/.dbeaver/data-sources.json:ro" \
  dbeaver/cloudbeaver

echo "CloudBeaver: http://127.0.0.1:8978"
