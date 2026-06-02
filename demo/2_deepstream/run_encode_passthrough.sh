#!/usr/bin/env bash
# Passthrough encode: RTSP -> mux -> NVENC -> RTSP (per-stream paths, no tile).
#
# Two-stream example (host playback uses 127.0.0.1; pull URLs use container DNS):
#   MAX_BATCH_SIZE=2 \
#   RTSP_URLS='rtsp://ai_stream2_mediamtx:8554/video1_B0,rtsp://ai_stream2_mediamtx:8554/video2_B0' \
#   RTSP_OUT_SUFFIX=_ai \
#   ./run_encode_passthrough.sh
#   -> rtsp://127.0.0.1:8554/video1_B0_ai  and  video2_B0_ai
#
# Swap mux sink pads (diagnostic: does花屏 follow source or demux src_0?):
#   PASSTHROUGH_MUX_SINK_ORDER=1,0 ... ./run_encode_passthrough.sh
#
# Force mux batch=MAX_BATCH_SIZE with fewer sources (e.g. b16 + 1 stream):
#   PASSTHROUGH_MUX_USE_MAX_BATCH=1 MAX_BATCH_SIZE=16 RTSP_URLS='.../video2_B0' ...
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"

CONTAINER="${DEEPSTREAM_CONTAINER_NAME:-ai_stream2_deepstream}"
MEDIAMTX="${MEDIAMTX_CONTAINER_NAME:-ai_stream2_mediamtx}"

RTSP_URLS="${RTSP_URLS:-rtsp://${MEDIAMTX}:8554/video1_B0}"
MAX_BATCH_SIZE="${MAX_BATCH_SIZE:-16}"

command -v docker >/dev/null || { echo "ERROR: docker required" >&2; exit 1; }

docker ps --format '{{.Names}}' | grep -qx "${CONTAINER}" || {
  echo "ERROR: container ${CONTAINER} is not running." >&2
  exit 1
}

SUFFIX="${RTSP_OUT_SUFFIX:-_ai}"

_use_max="${PASSTHROUGH_MUX_USE_MAX_BATCH:-}"
if [[ "${_use_max}" == "1" || "${_use_max}" == "true" || "${_use_max}" == "yes" ]]; then
  echo "=> Passthrough encode (mux batch=${MAX_BATCH_SIZE}, sources<batch OK)"
else
  echo "=> Passthrough encode (mux batch=min(sources,${MAX_BATCH_SIZE}), per-stream RTSP)"
fi
unset _use_max
[[ -n "${PASSTHROUGH_MUX_SINK_ORDER:-}" ]] && \
  echo "   mux sink order: ${PASSTHROUGH_MUX_SINK_ORDER} (s0->sink_${PASSTHROUGH_MUX_SINK_ORDER%%,*}, ...)"
echo "   in:  ${RTSP_URLS}"
echo "   out (host playback, path = {input_stem}${SUFFIX} per stream):"
IFS=',' read -r -a _rtsp_list <<< "${RTSP_URLS}"
for _u in "${_rtsp_list[@]}"; do
  _u="${_u// /}"
  [[ -z "${_u}" ]] && continue
  _stem="${_u##*/}"
  _stem="${_stem%%\?*}"
  echo "     rtsp://127.0.0.1:8554/${_stem}${SUFFIX}"
  echo "     http://127.0.0.1:8889/${_stem}${SUFFIX}"
done
unset _rtsp_list _u _stem
echo "   Ctrl+C to stop."
echo

DOCKER_TTY=()
if [[ -t 1 ]]; then
  DOCKER_TTY=(-it)
fi

exec docker exec "${DOCKER_TTY[@]}" \
  -e RTSP_URLS="${RTSP_URLS}" \
  -e MEDIAMTX_HOST="${MEDIAMTX}" \
  -e MEDIAMTX_RTSP_PORT="${MEDIAMTX_RTSP_PORT:-8554}" \
  -e MAX_BATCH_SIZE="${MAX_BATCH_SIZE}" \
  -e RTSP_OUT_SUFFIX="${SUFFIX}" \
  -e PASSTHROUGH_MUX_USE_MAX_BATCH="${PASSTHROUGH_MUX_USE_MAX_BATCH:-}" \
  -e PASSTHROUGH_MUX_SINK_ORDER="${PASSTHROUGH_MUX_SINK_ORDER:-}" \
  -e PASSTHROUGH_MUX_SYNC_INPUTS="${PASSTHROUGH_MUX_SYNC_INPUTS:-}" \
  -e PASSTHROUGH_DEMUX_PAD0_EXTRA_BUFFERS="${PASSTHROUGH_DEMUX_PAD0_EXTRA_BUFFERS:-}" \
  -w /app \
  "${CONTAINER}" \
  python3 encode_passthrough.py
