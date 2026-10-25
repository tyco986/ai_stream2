#pragma once

#include <cstddef>

namespace service_log {

// paths
inline const char* PATH_LIST = "/ai_stream2/backend/logs";
inline const char* PATH_ENABLE = "/ai_stream2/backend/logs/enable";
inline const char* PATH_DISABLE = "/ai_stream2/backend/logs/disable";
inline const char* PATH_STATUS = "/ai_stream2/backend/logs/status";

// messages
inline const char* MSG_ENABLE_OK = "log_enable_ok";
inline const char* MSG_DISABLE_OK = "log_disable_ok";
inline const char* MSG_STATUS_OK = "log_status_ok";
inline const char* MSG_GET_OK = "log_get_ok";
inline const char* MSG_GET_ERROR = "log_get_error";
inline const char* MSG_REMOVE_OK = "log_remove_ok";
inline const char* MSG_REMOVE_ERROR = "log_remove_error";

// query
inline const char* QUERY_DATE = "date";
inline const char* QUERY_PAGE = "page";
inline const std::size_t DATE_LENGTH = 10;

// files
inline const char* DIR = "/root/logs/backend";
inline const char* STREAM_DIR = "stream";
inline const char* STREAM_GROUP_DIR = "stream_group";
inline const char* MODEL_DIR = "model";
inline const char* PIPELINE_DIR = "pipeline";
inline const char* PIPELINE_GIE_DIR = "pipeline_gie";
inline const char* PIPELINE_ANALYZER_DIR = "pipeline_analyzer";
inline const char* SERVER_DIR = "server";
inline const char* PREVIEW_DIR = "preview";
inline const char* USER_GROUP_DIR = "user_group";
inline const char* USER_DIR = "user";
inline const char* EVENT_DIR = "event";
inline const char* NOTICE_DIR = "notice";
inline const char* MEDIA_DIR = "media";
inline const char* LOG_DIR = "log";
inline const char* PAGES[] = {
    STREAM_DIR,
    STREAM_GROUP_DIR,
    MODEL_DIR,
    PIPELINE_DIR,
    PIPELINE_GIE_DIR,
    PIPELINE_ANALYZER_DIR,
    SERVER_DIR,
    PREVIEW_DIR,
    USER_GROUP_DIR,
    USER_DIR,
    EVENT_DIR,
    NOTICE_DIR,
    MEDIA_DIR,
    LOG_DIR,
};
inline const char* FILE_SUFFIX = ".jsonl";
inline const std::size_t MAX_DAYS = 90;
inline const int ROTATION_HOUR = 0;
inline const int ROTATION_MINUTE = 0;

// keys
inline const char* KEY_ENABLED = "enabled";
inline const char* KEY_CONTENT = "content";
inline const char* KEY_TS = "ts";
inline const char* KEY_METHOD = "method";
inline const char* KEY_URL = "url";
inline const char* KEY_HEADER = "header";
inline const char* HEADER_AUTHORIZATION = "authorization";
inline const char* HEADER_ACCEPT_LANGUAGE = "accept-language";
inline const char* HEADERS[] = {
    HEADER_AUTHORIZATION,
    HEADER_ACCEPT_LANGUAGE,
};
inline const std::size_t HEADER_COUNT = sizeof(HEADERS) / sizeof(HEADERS[0]);
inline const char* KEY_CODE = "code";
inline const char* KEY_INPUT = "input";
inline const char* KEY_OUTPUT = "output";
inline const char* KEY_LATENCY = "latency";
inline const int LATENCY_DIGITS = 2;
inline const char* KEY_IP = "ip";
inline const char* KEY_IN_BYTES = "in_bytes";
inline const char* KEY_OUT_BYTES = "out_bytes";
inline const char* KEY_ERROR = "error";
inline const char* LINE_KEYS[] = {
    KEY_TS,
    KEY_METHOD,
    KEY_URL,
    KEY_CODE,
    KEY_INPUT,
    KEY_OUTPUT,
    KEY_LATENCY,
    KEY_IP,
    KEY_IN_BYTES,
    KEY_OUT_BYTES,
    KEY_ERROR,
    KEY_HEADER,
};
inline const std::size_t LINE_KEY_COUNT = sizeof(LINE_KEYS) / sizeof(LINE_KEYS[0]);

}
