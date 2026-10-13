#pragma once

#include <nlohmann/json.hpp>

namespace exporter {

inline constexpr const char* EXPORT_PATH = "/ai_stream2/export_trt/export";
inline constexpr const char* FIELD_INPUT = "input";
inline constexpr const char* FIELD_CONFIG = "config";
inline constexpr const char* TMP_DIR = "/root/tmp";
inline constexpr const char* META_JSON = "meta.json";
inline constexpr const char* LABELS_TXT = "labels.txt";
inline constexpr const char* ENGINE_SUFFIX = ".engine";
inline constexpr const char* ZIP_SUFFIX = ".zip";
inline constexpr const char* STATUS_BUILT = "built";
inline constexpr const char* STATUS_NOT_BUILT = "not_built";
inline constexpr int MIN_SHAPE = 1;
inline constexpr const char* BATCH_MODE_DYNAMIC = "dynamic";
inline constexpr const char* BATCH_MODE_STATIC = "static";

inline const nlohmann::json STATIC_PLUGIN = {
    {"YOLO10-DET", nullptr},
    {"YOLO11-DET", nullptr},
    {"YOLO11-SEG", "/opt/ai_stream2/servers/export_trt/libs/libnvdsinfer_custom_impl_Yolo_seg.so"},
    {"YOLO26-DET", nullptr},
    {"YOLO26-POSE", nullptr},
    {"YOLO26-SEG", "/opt/ai_stream2/servers/export_trt/libs/libnvdsinfer_custom_impl_Yolo_seg.so"},
    {"RTMPOSE", nullptr},
    {"STGCNPP", nullptr},
};

inline const nlohmann::json RESULT = {
    {"type", nullptr},
    {"batch_size", 1},
    {"batch_mode", BATCH_MODE_DYNAMIC},
    {"input_tensor_name", nullptr},
    {"input_tensor_shape", nullptr},
    {"precision", "fp16"},
    {"opt_level", nullptr},
    {"device", 0},
    {"workspace", 8192},
    {"static_plugin", nullptr},
    {"max_det", nullptr},
    {"conf", nullptr},
    {"iou", nullptr},
    {"num_class", nullptr},
    {"classes", nullptr},
    {"status", "not_built"},
    {"built_at", nullptr},
    {"trt_version", nullptr},
};

}
