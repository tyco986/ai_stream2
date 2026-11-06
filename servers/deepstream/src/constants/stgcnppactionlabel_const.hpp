#pragma once

#include "nvdsmeta.h"

namespace stgcnppactionlabel {

inline constexpr NvDsMetaType USER_META = static_cast<NvDsMetaType>(NVDS_START_USER_META + 0x5341);
inline constexpr int DEFAULT_CLASSIFIER_UNIQUE_ID = 4;
inline constexpr float CONF_NONE = 0.0f;
inline constexpr int CLASS_NONE = -1;
inline constexpr const char* LABEL_NA = "NA";
inline constexpr const char* LABEL_UNKNOWN = "Unknown";

inline constexpr const char* ELEMENT_NAME = "stgcnppactionlabel";
inline constexpr const char* ELEMENT_DETAIL = "Collect ST-GCN++ classifier results into object user meta";
inline constexpr const char* PLUGIN_DESCRIPTION = "DeepStream ST-GCN++ action label";
inline constexpr const char* VERSION = "1.0";
inline constexpr const char* LICENSE = "Apache-2.0";
inline constexpr const char* PLUGIN_PACKAGE = "DeepStream ST-GCN++ action label";
inline constexpr const char* ORIGIN = "https://github.com";

}

#define STGCNPPACTIONLABEL_NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
