#pragma once

#include <nlohmann/json.hpp>

namespace exporter {

inline constexpr const char* EXPORT_PATH = "/ai_stream2/export_trt/export";
inline constexpr const char* FIELD_INPUT = "input";
inline constexpr const char* FIELD_CONFIG = "config";
inline constexpr const char* META_JSON = "meta.json";
inline constexpr const char* LABELS_TXT = "labels.txt";
inline constexpr const char* ONNX_SUFFIX = ".onnx";
inline constexpr const char* ENGINE_SUFFIX = ".engine";
inline constexpr const char* ZIP_SUFFIX = ".zip";
inline constexpr const char* TRTEXEC = "/usr/src/tensorrt/bin/trtexec";
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

inline constexpr const char* KEY_TYPE = "type";
inline constexpr const char* KEY_BATCH_SIZE = "batch_size";
inline constexpr const char* KEY_BATCH_MODE = "batch_mode";
inline constexpr const char* KEY_INPUT_TENSORS = "input_tensors";
inline constexpr const char* KEY_OUTPUT_TENSORS = "output_tensors";
inline constexpr const char* KEY_PRECISION = "precision";
inline constexpr const char* KEY_OPT_LEVEL = "opt_level";
inline constexpr const char* KEY_DEVICE = "device";
inline constexpr const char* KEY_WORKSPACE = "workspace";
inline constexpr const char* KEY_STATIC_PLUGIN = "static_plugin";
inline constexpr const char* KEY_MAX_DET = "max_det";
inline constexpr const char* KEY_CONF = "conf";
inline constexpr const char* KEY_IOU = "iou";
inline constexpr const char* KEY_NUM_CLASS = "num_class";
inline constexpr const char* KEY_CLASSES = "classes";
inline constexpr const char* KEY_STATUS = "status";
inline constexpr const char* KEY_BUILT_AT = "built_at";
inline constexpr const char* KEY_TRT_VERSION = "trt_version";

inline const nlohmann::json RESULT = {
    {KEY_TYPE, nullptr},
    {KEY_BATCH_SIZE, 1},
    {KEY_BATCH_MODE, BATCH_MODE_DYNAMIC},
    {KEY_INPUT_TENSORS, nullptr},
    {KEY_OUTPUT_TENSORS, nullptr},
    {KEY_PRECISION, "fp16"},
    {KEY_OPT_LEVEL, nullptr},
    {KEY_DEVICE, 0},
    {KEY_WORKSPACE, 8192},
    {KEY_STATIC_PLUGIN, nullptr},
    {KEY_MAX_DET, nullptr},
    {KEY_CONF, nullptr},
    {KEY_IOU, nullptr},
    {KEY_NUM_CLASS, nullptr},
    {KEY_CLASSES, nullptr},
    {KEY_STATUS, STATUS_NOT_BUILT},
    {KEY_BUILT_AT, nullptr},
    {KEY_TRT_VERSION, nullptr},
};

}
