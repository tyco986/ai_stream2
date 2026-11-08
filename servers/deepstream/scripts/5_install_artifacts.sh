#!/usr/bin/env bash
set -euo pipefail

BUILD="${1:-/tmp/build}"
mkdir -p /app /opt/ai_stream2/servers/deepstream/libs
cp "${BUILD}/deepstream" /app/deepstream
cp "${BUILD}/src/plugins/boxsnapshot/libgstboxsnapshot.so" "${GST_PLUGIN_PATH}/"
cp "${BUILD}/src/plugins/fadedrawer/libgstfadedrawer.so" "${GST_PLUGIN_PATH}/"
cp "${BUILD}/src/plugins/presenceeventcoder/libgstpresenceeventcoder.so" "${GST_PLUGIN_PATH}/"
cp "${BUILD}/src/plugins/capturer/libgstcapturer.so" "${GST_PLUGIN_PATH}/"
cp "${BUILD}/src/plugins/dslogger/libgstdslogger.so" "${GST_PLUGIN_PATH}/"
cp "${BUILD}/src/plugins/stgcnppactionlabel/libgststgcnppactionlabel.so" "${GST_PLUGIN_PATH}/"
cp "${BUILD}/src/plugins/sahipostprocesspose/libgstnvsahipostprocesspose.so" "${GST_PLUGIN_PATH}/"
cp "${BUILD}/src/plugins/nvdspreprocess/rtmposepreprocess/libnvds_rtmpose_preprocess.so" /opt/ai_stream2/servers/deepstream/libs/
cp "${BUILD}/src/plugins/nvdspreprocess/stgcnpppreprocess/libnvds_stgcnpp_preprocess.so" /opt/ai_stream2/servers/deepstream/libs/
cp "${BUILD}/src/plugins/nvinfer/yolopose/libnvinfer_yolo_pose.so" /opt/ai_stream2/servers/deepstream/libs/
cp "${BUILD}/src/plugins/nvmsgconv/libnvds_msgconv_det.so" /opt/ai_stream2/servers/deepstream/libs/
cp "${BUILD}/src/plugins/nvmsgconv/libnvds_msgconv_presence.so" /opt/ai_stream2/servers/deepstream/libs/
cp "${BUILD}/src/plugins/nvmsgconv/libnvds_msgconv_stgcnpp.so" /opt/ai_stream2/servers/deepstream/libs/
