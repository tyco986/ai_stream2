#pragma once

namespace request_log {

inline constexpr const char* ROOT = "/root/logs/export_trt";
inline constexpr const char* PAGE = "export_trt";
inline constexpr const char* SUFFIX = ".jsonl";
inline constexpr int MAX_DAYS = 90;
inline constexpr const char* MULTIPART_OUTPUT = "multipart(zip)";
inline constexpr const char* HEADERS[] = {"authorization", "accept-language"};
inline constexpr const char* STARTED = "started";
inline constexpr const char* KEY_TS = "ts";
inline constexpr const char* KEY_METHOD = "method";
inline constexpr const char* KEY_URL = "url";
inline constexpr const char* KEY_CODE = "code";
inline constexpr const char* KEY_INPUT = "input";
inline constexpr const char* KEY_OUTPUT = "output";
inline constexpr const char* KEY_LATENCY = "latency";
inline constexpr const char* KEY_IP = "ip";
inline constexpr const char* KEY_IN_BYTES = "in_bytes";
inline constexpr const char* KEY_OUT_BYTES = "out_bytes";
inline constexpr const char* KEY_ERROR = "error";
inline constexpr const char* KEY_HEADER = "header";
inline constexpr const char* LATENCY_PLACEHOLDER = "\"latency\":0";

}
