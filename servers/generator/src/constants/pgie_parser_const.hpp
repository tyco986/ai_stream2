#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace pgie_parser {

inline constexpr const char* MODELS_DIR = "/root/runtime/models";
inline constexpr const char* META_JSON_NAME = "meta.json";
inline constexpr const char* LABELS_NAME = "labels.txt";

inline const std::unordered_map<std::string, int> NETWORK_MODE_MAP = {
    {"fp32", 0},
    {"fp16", 2},
    {"int8", 1},
};

inline const std::unordered_set<std::string> SUPPORTED_TYPES = {
    "YOLO10-DET",
    "YOLO11-DET",
    "YOLO11-SEG",
    "YOLO26-DET",
    "YOLO26-POSE",
    "YOLO26-SEG",
};

inline const std::unordered_map<std::string, std::string> CLASS_ATTR_KEY_MAP = {
    {"conf", "pre-cluster-threshold"},
    {"topk", "topk"},
    {"detected_min_w", "detected-min-w"},
    {"detected_min_h", "detected-min-h"},
    {"detected_max_w", "detected-max-w"},
    {"detected_max_h", "detected-max-h"},
};

}
