#!/usr/bin/env bash
# Run from project root.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
# shellcheck source=../../../scripts/load_project_env.sh
source "${ROOT}/scripts/load_project_env.sh"

NAME="${PROJECT_NAME}_mmdeploy"
IMAGE="openmmlab/mmdeploy:ubuntu20.04-cuda11.8-mmdeploy1.3.1"

mkdir -p "${ROOT}/models" "${ROOT}/logs"

docker network create "${PROJECT_NAME}_default" 2>/dev/null || true
docker rm -f "${NAME}" 2>/dev/null || true

docker run -d \
  --name "${NAME}" \
  --network "${PROJECT_NAME}_default" \
  -e TZ="${TZ:-$(cat /etc/timezone 2>/dev/null || echo UTC)}" \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${ROOT}/models:/root/models" \
  -v "${ROOT}/logs:/root/logs" \
  -v "${ROOT}/servers/mmdeploy:/opt/mmdeploy-src" \
  "${IMAGE}" \
  sleep infinity

docker exec "${NAME}" python3 -m pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple
docker exec "${NAME}" python3 -m pip install -U openmim joblib
docker exec "${NAME}" python3 -m mim install "mmdet>=3.0.0" "mmpose>=1.3.0" "mmrazor>=1.0.0"
docker exec -i "${NAME}" python3 - <<'PY'
from pathlib import Path

root = Path("/usr/local/lib/python3.8/dist-packages/mmrazor")
models_init = root / "models" / "__init__.py"
text = models_init.read_text(encoding="utf-8")
for line in [
    "from .fake_quants import *  # noqa: F401,F403\n",
    "from .observers import *  # noqa: F401,F403\n",
    "from .quantizers import *  # noqa: F401,F403\n",
]:
    text = text.replace(line, "")
models_init.write_text(text, encoding="utf-8")
algo_init = root / "models" / "algorithms" / "__init__.py"
text = algo_init.read_text(encoding="utf-8")
text = text.replace("from .quantization import MMArchitectureQuant, MMArchitectureQuantDDP\n", "")
text = text.replace(", 'MMArchitectureQuant',\n    'MMArchitectureQuantDDP'", "")
algo_init.write_text(text, encoding="utf-8")
setup_env = root / "utils" / "setup_env.py"
text = setup_env.read_text(encoding="utf-8")
setup_env.write_text(
    text.replace("    import mmrazor.engine  # noqa: F401,F403\n", ""),
    encoding="utf-8",
)
demo = root / "models" / "task_modules" / "demo_inputs" / "mmpose_demo_input.py"
demo.write_text(
    demo.read_text(encoding="utf-8").replace("RTMHead", "RTMCCHead"),
    encoding="utf-8",
)
PY

echo "MMDeploy: ${NAME} image=${IMAGE}"
