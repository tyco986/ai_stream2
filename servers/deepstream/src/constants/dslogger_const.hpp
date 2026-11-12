#pragma once

#include <cstddef>

#include "nvdsmeta.h"

namespace dslogger {

inline constexpr const char* ROOT = "/root/logs/deepstream";
inline constexpr const char* SUFFIX = ".jsonl";
inline constexpr const char* PAGE_DET = "det";
inline constexpr const char* PAGE_PRESENCE = "presence";
inline constexpr const char* PAGE_STGCNPP = "stgcnpp";
inline constexpr std::size_t MAX_BYTES = 10 * 1024 * 1024;
inline constexpr std::size_t MAX_FILES = 5;
inline constexpr int PAD_WIDTH = 3;
inline constexpr int DEFAULT_INTERVAL = 1;
inline constexpr const char* TS_FORMAT = "%Y-%m-%d %H:%M:%S";
inline constexpr int TS_SIZE = 32;
inline constexpr double CONF_SCALE = 100.0;
inline constexpr int CONF_DIGITS = 2;
inline constexpr int MISSING_OBJECT_ID = -1;
inline constexpr double LATENCY_NONE = -1.0;
inline constexpr const char* JSON_NULL = "null";
inline constexpr float ACTION_CONF_NONE = 0.0f;
inline constexpr int ACTION_CLASS_NONE = -1;
inline constexpr NvDsMetaType PRESENCE_EVENT_USER_META = static_cast<NvDsMetaType>(NVDS_START_USER_META + 0x5045);
inline constexpr unsigned PRESENCE_EVENT_CODE_LEN = 8;
inline constexpr unsigned PRESENCE_EVENT_NAME_LEN = 64;
inline constexpr NvDsMetaType STGCNPP_ACTION_USER_META = static_cast<NvDsMetaType>(NVDS_START_USER_META + 0x5341);
inline constexpr NvDsMetaType STGCNPP_READY_USER_META = static_cast<NvDsMetaType>(NVDS_START_USER_META + 0x5347);

inline constexpr const char* ELEMENT_DET = "detlogger";
inline constexpr const char* ELEMENT_PRESENCE = "presencelogger";
inline constexpr const char* ELEMENT_STGCNPP = "stgcnpplogger";
inline constexpr const char* ELEMENT_DET_DETAIL = "Write detection boxes as JSON lines";
inline constexpr const char* ELEMENT_PRESENCE_DETAIL = "Write detection boxes and presence event codes as JSON lines";
inline constexpr const char* ELEMENT_STGCNPP_DETAIL = "Write detection and ST-GCN++ action fields as JSON lines";
inline constexpr const char* PLUGIN_DESCRIPTION = "DeepStream object logger elements";
inline constexpr const char* VERSION = "1.0";
inline constexpr const char* LICENSE = "Apache-2.0";
inline constexpr const char* PLUGIN_PACKAGE = "DeepStream object logger";
inline constexpr const char* ORIGIN = "https://github.com";

}

#define DSLOGGER_NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
