#pragma once

#include "nvdsmeta.h"

namespace presenceeventcoder {

inline constexpr int CODE_BOUNDS = 3;
inline constexpr int DEFAULT_LENGTH = 10;
inline constexpr float DEFAULT_THRESHOLD = 0.5f;
inline constexpr const char* DEFAULT_MODE = "fold";
inline constexpr NvDsMetaType USER_META = static_cast<NvDsMetaType>(NVDS_START_USER_META + 0x5045);

inline constexpr const char* ELEMENT_NAME = "presenceeventcoder";
inline constexpr const char* ELEMENT_DETAIL = "Encode presence event codes into frame user meta";
inline constexpr const char* PLUGIN_DESCRIPTION = "DeepStream event coder elements";
inline constexpr const char* VERSION = "1.0";
inline constexpr const char* LICENSE = "Apache-2.0";
inline constexpr const char* PLUGIN_PACKAGE = "DeepStream event coder";
inline constexpr const char* ORIGIN = "https://github.com";

}

#define PRESENCEEVENTCODER_NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
