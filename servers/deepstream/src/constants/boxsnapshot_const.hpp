#pragma once

#include "nvdsmeta.h"

namespace boxsnapshot {

inline constexpr NvDsMetaType USER_META = static_cast<NvDsMetaType>(NVDS_START_USER_META + 0x4253);
inline constexpr const char* ELEMENT_NAME = "boxsnapshot";
inline constexpr const char* ELEMENT_DETAIL = "Copy object boxes into frame user meta";
inline constexpr const char* PLUGIN_DESCRIPTION = "DeepStream frame bbox snapshot";
inline constexpr const char* VERSION = "1.0";
inline constexpr const char* LICENSE = "Apache-2.0";
inline constexpr const char* PLUGIN_PACKAGE = "DeepStream bbox snapshot";
inline constexpr const char* ORIGIN = "https://github.com";

}

#define BOXSNAPSHOT_NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
