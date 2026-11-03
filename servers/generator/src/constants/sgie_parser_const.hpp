#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace sgie_parser {

inline constexpr const char* RUNTIME_ROOT = "/root/runtime";
inline constexpr const char* META_JSON_NAME = "meta.json";
inline constexpr const char* LABELS_NAME = "labels.txt";

inline const std::unordered_map<std::string, int> NETWORK_MODE_MAP = {
    {"fp32", 0},
    {"fp16", 2},
    {"int8", 1},
};

inline const std::unordered_set<std::string> SUPPORTED_TYPES = {
    "RTMPOSE",
};

}
