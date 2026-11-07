#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace stgcnpp_parser {

inline constexpr const char* MODELS_DIR = "/root/runtime/models";
inline constexpr const char* META_JSON_NAME = "meta.json";
inline constexpr const char* LABELS_NAME = "labels.txt";
inline constexpr std::array<int, 4> INPUT_TAIL = {2, 100, 17, 3};

inline const std::unordered_map<std::string, int> NETWORK_MODE_MAP = {
    {"fp32", 0},
    {"fp16", 2},
    {"int8", 1},
};

inline const std::unordered_set<std::string> SUPPORTED_TYPES = {
    "STGCNPP",
};

inline const std::unordered_set<std::string> SUPPORTED_BATCH_MODES = {
    "static",
    "dynamic",
};

}
