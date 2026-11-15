#pragma once

#include <cstddef>

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

inline constexpr const char* ELEMENT_NAME = "dslogger";
inline constexpr const char* ELEMENT_DETAIL = "Write collector JSON lines to disk";
inline constexpr const char* PLUGIN_DESCRIPTION = "DeepStream JSON line logger";
inline constexpr const char* VERSION = "1.0";
inline constexpr const char* LICENSE = "Apache-2.0";
inline constexpr const char* PLUGIN_PACKAGE = "DeepStream object logger";
inline constexpr const char* ORIGIN = "https://github.com";

}

#define DSLOGGER_NVMM_CAPS GST_VIDEO_CAPS_MAKE_WITH_FEATURES("memory:NVMM", "{ NV12, RGBA, I420 }")
