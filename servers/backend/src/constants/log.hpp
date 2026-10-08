#pragma once

#include <cstddef>

namespace log {

// paths
inline const char* PATH_LIST = "/ai_stream2/backend/logs";
inline const char* PATH_ENABLE = "/ai_stream2/backend/logs/enable";

// messages
inline const char* MSG_ENABLE_OK = "Enabled debug log";
inline const char* MSG_GET_OK = "Loaded debug log";
inline const char* MSG_GET_ERROR = "Failed to load debug log";

// query
inline const char* QUERY_DATE = "date";
inline const char* QUERY_PAGE = "page";
inline const std::size_t DATE_LENGTH = 10;

// files
inline const char* DIR = "/root/logs/backend";
inline const char* STREAM_DIR = "stream";
inline const char* STREAM_GROUP_DIR = "stream_group";
inline const char* FILE_SUFFIX = ".json";
inline const std::size_t MAX_DAYS = 90;
inline const int ROTATION_HOUR = 0;
inline const int ROTATION_MINUTE = 0;

// keys
inline const char* KEY_CONTENT = "content";
inline const char* KEY_TS = "ts";
inline const char* KEY_METHOD = "method";
inline const char* KEY_URL = "url";
inline const char* KEY_CODE = "code";
inline const char* KEY_INPUT = "input";
inline const char* KEY_OUTPUT = "output";
inline const char* KEY_LATENCY = "latency";
inline const char* KEY_IP = "ip";
inline const char* KEY_IN_BYTES = "in_bytes";
inline const char* KEY_OUT_BYTES = "out_bytes";

}
