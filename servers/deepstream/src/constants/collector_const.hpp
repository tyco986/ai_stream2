#pragma once

#include "nvdsmeta.h"

namespace collector {

inline constexpr NvDsMetaType USER_META = static_cast<NvDsMetaType>(NVDS_START_USER_META + 0x434C);
inline constexpr const char* TS_FORMAT = "%Y-%m-%d %H:%M:%S";
inline constexpr int TS_SIZE = 32;
inline constexpr double CONF_SCALE = 100.0;
inline constexpr int CONF_DIGITS = 2;
inline constexpr int MISSING_OBJECT_ID = -1;
inline constexpr double LATENCY_NONE = -1.0;
inline constexpr int DEFAULT_INTERVAL = 1;
inline constexpr const char* JSON_NULL = "null";
inline constexpr const char* DEFAULT_DUMP_CODES = "12";
inline constexpr const char* PROP_DUMP_CODES = "dump-codes";
inline constexpr const char* RAW_RELATIVE = "images/raw_{:03d}_{:08d}.png";
inline constexpr const char* VIS_RELATIVE = "vis/vis_{:03d}_{:08d}.jpg";
inline constexpr const char* PROP_CAPTURER_OUTPUT = "capturer-output";
inline constexpr const char* KEY_EVENT = "event";
inline constexpr const char* KEY_RAW_PATH = "raw_path";
inline constexpr const char* KEY_VISUALIZATION_PATH = "visualization_path";

inline constexpr const char* ELEMENT_DET = "detcollector";
inline constexpr const char* ELEMENT_PRESENCE = "presencecollector";
inline constexpr const char* ELEMENT_STGCNPP = "stgcnppcollector";
inline constexpr const char* ELEMENT_DET_DETAIL = "Snapshot detection JSON onto frame user meta";
inline constexpr const char* ELEMENT_PRESENCE_DETAIL = "Snapshot presence JSON onto frame user meta";
inline constexpr const char* ELEMENT_STGCNPP_DETAIL = "Snapshot ST-GCN++ JSON onto frame user meta";
inline constexpr const char* PLUGIN_DESCRIPTION = "DeepStream frame JSON collectors";
inline constexpr const char* VERSION = "1.0";
inline constexpr const char* LICENSE = "Apache-2.0";
inline constexpr const char* PLUGIN_PACKAGE = "DeepStream frame collector";
inline constexpr const char* ORIGIN = "https://github.com";

}

#define COLLECTOR_NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
