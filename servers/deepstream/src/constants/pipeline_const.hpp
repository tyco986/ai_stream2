#pragma once

#include <cstddef>

namespace pipeline {

inline constexpr const char* START_PATH = "/start";
inline constexpr const char* START_LOCAL_PATH = "/start/local";
inline constexpr const char* STOP_PATH = "/stop";
inline constexpr const char* STATUS_PATH = "/status";
inline constexpr const char* KEY_RUNNING = "running";
inline constexpr const char* KEY_STATUS = "status";
inline constexpr const char* KEY_START_TIME = "start_time";
inline constexpr const char* KEY_END_TIME = "end_time";
inline constexpr const char* KEY_DURATION = "duration";
inline constexpr const char* STATUS_RUNNING = "running";
inline constexpr const char* STATUS_STOPPED = "stopped";
inline constexpr const char* TIME_FORMAT = "%Y-%m-%dT%H:%M:%S%z";
inline constexpr const char* FIELD_CONFIG = "config";
inline constexpr const char* KEY_MODEL_ZIP = "model_zip";
inline constexpr const char* FIELD_MODEL = "model";
inline constexpr const char* FIELD_MEDIA = "media";
inline constexpr const char* RUNTIME_ROOT = "/root/runtime";
inline constexpr const char* CONFIG_DIR = "/root/runtime/config";
inline constexpr const char* MODELS_DIR = "/root/runtime/models";
inline constexpr int STATUS_CONFLICT = 409;
inline constexpr const char* ERR_RUNNING = "stop the pipeline before start";
inline constexpr const char* ERR_IDLE = "pipeline is not running";
inline constexpr std::size_t READ_BUFFER = 64 * 1024;
inline constexpr std::size_t PAYLOAD_MAX = 2ULL * 1024 * 1024 * 1024;

}
