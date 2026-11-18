#pragma once

#include <cstddef>

namespace gstreamer_log {

inline constexpr const char* ROOT = "/root/logs/deepstream";
inline constexpr const char* DIRECTORY = "gstreamer";
inline constexpr const char* FILE_NAME = "gstreamer.log";
inline constexpr const char* FILE_PREFIX = "gstreamer_";
inline constexpr const char* FILE_SUFFIX = ".log";
inline constexpr const char* DATE_FORMAT = "%Y-%m-%d";
inline constexpr std::size_t DATE_LENGTH = 10;
inline constexpr std::size_t DATE_SIZE = 11;
inline constexpr std::size_t MAX_DAYS = 90;
inline constexpr const char* LEGACY_FILE = "pipeline.log";
inline constexpr int REOPEN_SECONDS = 1;
inline constexpr char DROP_PREFIX[] = "Encode Latency";
inline constexpr std::size_t READ_SIZE = 4096;
inline constexpr std::size_t MAX_LINE = 65536;

}
