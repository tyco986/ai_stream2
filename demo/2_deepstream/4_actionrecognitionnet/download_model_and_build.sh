#!/usr/bin/env bash
# Run on host: download ONNX model + build preprocess .so in DeepStream container.
#
# Usage: ./download_model_and_build.sh --ngc-api-key <KEY>
#        NGC_API_KEY=<KEY> ./download_model_and_build.sh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
MODELS_DIR="${ROOT}/deepstream/models"
LIB_OUT="${ROOT}/deepstream/lib/libnvds_custom_sequence_preprocess.so"
ZIP="${MODELS_DIR}/actionrecognitionnet_deployable_onnx_v1.0.zip"
ONNX_URL="https://api.ngc.nvidia.com/v2/models/nvidia/tao/actionrecognitionnet/versions/deployable_onnx_v1.0/zip"
ONNX_3D="${MODELS_DIR}/resnet18_3d_rgb_hmdb5_32.onnx"
CONTAINER="${DEEPSTREAM_CONTAINER_NAME:-ai_stream2_deepstream}"
IMAGE="${DEEPSTREAM_BASE_IMAGE:-nvcr.io/nvidia/deepstream:9.0-triton-multiarch}"
CUDA_VER="${CUDA_VER:-13.1}"

NGC_API_KEY_ARG=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --ngc-api-key) NGC_API_KEY_ARG="${2:?}"; shift 2 ;;
    *) echo "Usage: $0 --ngc-api-key <KEY>" >&2; exit 1 ;;
  esac
done
NGC_API_KEY="${NGC_API_KEY_ARG:-${NGC_API_KEY:-}}"
[[ -n "${NGC_API_KEY}" ]] || { echo "ERROR: --ngc-api-key or NGC_API_KEY required" >&2; exit 1; }

command -v docker >/dev/null || { echo "ERROR: docker required" >&2; exit 1; }
command -v curl >/dev/null || { echo "ERROR: curl required" >&2; exit 1; }
command -v unzip >/dev/null || { echo "ERROR: unzip required" >&2; exit 1; }

if [[ -f "${ONNX_3D}" ]]; then
  echo "=> ONNX already present: ${ONNX_3D}"
else
  echo "=> Download deployable_onnx_v1.0 to ${MODELS_DIR}"
  mkdir -p "${MODELS_DIR}" "${ROOT}/deepstream/lib"
  curl -fSL -H "Authorization: Bearer ${NGC_API_KEY}" -o "${ZIP}" "${ONNX_URL}"
  unzip -o "${ZIP}" -d "${MODELS_DIR}"
  rm -f "${ZIP}"
fi

ls -la "${ONNX_3D}"

BUILD='
set -euo pipefail
for SAMPLE in \
  /opt/nvidia/deepstream/deepstream-9.0/sources/apps/sample_apps/deepstream-3d-action-recognition \
  /opt/nvidia/deepstream/deepstream/sources/apps/sample_apps/deepstream-3d-action-recognition; do
  [[ -f "${SAMPLE}/Makefile" ]] || continue
  cd "${SAMPLE}"
  export CUDA_VER='"${CUDA_VER}"'
  make
  if [[ $(id -u) -eq 0 ]]; then make install; else sudo -E make install; fi
  exit 0
done
echo "ERROR: deepstream-3d-action-recognition sample not found" >&2
exit 1
'

SO_PATHS=(
  "/opt/nvidia/deepstream/deepstream-9.0/lib/libnvds_custom_sequence_preprocess.so"
  "/opt/nvidia/deepstream/deepstream/lib/libnvds_custom_sequence_preprocess.so"
)

copy_so_from_container() {
  local name="$1"
  local path
  for path in "${SO_PATHS[@]}"; do
    if docker exec "${name}" test -f "${path}" 2>/dev/null; then
      docker cp "${name}:${path}" "${LIB_OUT}"
      return 0
    fi
  done
  return 1
}

echo "=> Build preprocess lib in container (CUDA_VER=${CUDA_VER})"
if docker ps --format '{{.Names}}' | grep -qx "${CONTAINER}"; then
  docker exec "${CONTAINER}" bash -c "${BUILD}"
  copy_so_from_container "${CONTAINER}"
else
  echo "   Container ${CONTAINER} not running, using ${IMAGE}"
  tmp="ds_action_build_$$"
  docker run --name "${tmp}" --gpus all "${IMAGE}" bash -c "${BUILD}"
  copy_so_from_container "${tmp}"
  docker rm -f "${tmp}"
fi

echo "=> ${LIB_OUT}"
ls -la "${LIB_OUT}"
