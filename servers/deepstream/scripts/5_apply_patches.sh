#!/bin/bash
set -euo pipefail

cd /opt/nvidia/deepstream/deepstream-9.0
patch -p0 < /patches/fix_sgie_global_interval.patch
patch -p0 < /patches/fix_nvinfer_interval_ineffectiveness_caused_by_nvdspreprocess.patch
make -C sources/gst-plugins/gst-nvinfer CUDA_VER=13.1 -j"$(nproc)"
make -C sources/gst-plugins/gst-nvinfer CUDA_VER=13.1 install

mkdir -p /root/src
unzip -o -q /tmp/deepstream-sahi-master.zip -d /root/src
cd /root/src/deepstream-sahi-master
patch -p1 < /patches/deepstream_sahi/fix_rotation_angle_for_libnvds_infer_yolo.patch
patch -p1 < /patches/deepstream_sahi/add_latency_to_nvsahipostprocess.patch
bash install.sh --plugins-only
