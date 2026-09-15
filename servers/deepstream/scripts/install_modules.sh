#!/usr/bin/env bash
# Build every CMakeLists under MODULES_ROOT and install .so / binaries.
# Image build: MODULES_ROOT=/tmp/modules (default CMAKE_BUILD_ROOT = <src>/build).
# Dev container: MODULES_ROOT=/app/modules CMAKE_BUILD_ROOT=/tmp/ds-cmake-build.
set -euo pipefail

modules_root="${MODULES_ROOT:?}"
gst_plugin_path="${GST_PLUGIN_PATH:?}"
libs_root="${DEEPSTREAM_LIBS_ROOT:?}"
cuda_ver="${CUDA_VER:-13.1}"

export PATH="/usr/local/cuda-${cuda_ver}/bin:${PATH}"
stub="/usr/local/cuda-${cuda_ver}/lib64/stubs"
ln -sf "${stub}/libcuda.so" "${stub}/libcuda.so.1"
export LIBRARY_PATH="${stub}${LIBRARY_PATH:+:${LIBRARY_PATH}}"

mkdir -p "${libs_root}" "${gst_plugin_path}"
mapfile -t cmake_lists < <(find "${modules_root}" -name CMakeLists.txt | sort)
test "${#cmake_lists[@]}" -gt 0
for cm in "${cmake_lists[@]}"; do
  src="$(dirname "$cm")"
  if [[ -n "${CMAKE_BUILD_ROOT:-}" ]]; then
    rel="${src#"${modules_root}/"}"
    bdir="${CMAKE_BUILD_ROOT}/${rel}"
  else
    bdir="${src}/build"
  fi
  cmake -S "$src" -B "$bdir"
  cmake --build "$bdir" -j"$(nproc)"
  shopt -s nullglob
  for so in "$bdir"/*.so; do
    name="$(basename "$so")"
    dest="${libs_root}"
    if [[ "$name" == libnvdsgst_* ]]; then dest="${gst_plugin_path}"; fi
    cp "$so" "${dest}/"
    test -f "${dest}/${name}"
  done
  for bin in "$bdir"/*; do
    if [[ -f "$bin" && -x "$bin" && "$bin" != *.so ]]; then
      name="$(basename "$bin")"
      cp "$bin" "/usr/local/bin/${name}"
      test -x "/usr/local/bin/${name}"
    fi
  done
done
test -x /usr/local/bin/pipeline_runner
test -x /usr/local/bin/deepstream_api
test -f "${libs_root}/libgsource_remove_guard.so"
