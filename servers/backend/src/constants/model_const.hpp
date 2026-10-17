#pragma once

#include <json/json.h>

#include <map>
#include <string>

namespace model {

// paths
inline const char* PATH_LIST = "/ai_stream2/backend/models";
inline const char* PATH_IMPORT = "/ai_stream2/backend/models/import";
inline const char* PATH_BATCH_BUILD = "/ai_stream2/backend/models/batch/build";
inline const char* PATH_BATCH_STATUS = "/ai_stream2/backend/models/batch/status";
inline const char* PATH_BATCH_REMOVE = "/ai_stream2/backend/models/batch/remove";
inline const char* PATH_ITEM = "/ai_stream2/backend/models/{model_id}";
inline const char* PATH_BUILD = "/ai_stream2/backend/models/{model_id}/build";
inline const char* PATH_STATUS = "/ai_stream2/backend/models/{model_id}/status";
inline const char* PATH_UPLOAD = "/ai_stream2/backend/models/{model_id}/upload";
inline const char* PATH_MAP = "/ai_stream2/backend/models/map";
inline const char* PATH_EXPORT_ONNX = "/ai_stream2/export_onnx/export";
inline const char* PATH_EXPORT_TRT = "/ai_stream2/export_trt/export";

// keys
inline const char* KEY_ITEMS = "items";
inline const char* KEY_IDS = "ids";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_FAILED_COUNT = "failed_count";
inline const char* KEY_SKIPPED_COUNT = "skipped_count";
inline const char* KEY_UPDATED_ROWS = "updated_rows";
inline const char* KEY_FAILED_ROWS = "failed_rows";
inline const char* KEY_SKIPPED_ROWS = "skipped_rows";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";
inline const char* KEY_ID = "id";
inline const char* KEY_NAME = "name";
inline const char* KEY_TYPE = "type";
inline const char* KEY_VERSION = "version";
inline const char* KEY_BATCH_SIZE = "batch_size";
inline const char* KEY_BATCH_MODE = "batch_mode";
inline const char* KEY_INPUT_TENSORS = "input_tensors";
inline const char* KEY_OUTPUT_TENSORS = "output_tensors";
inline const char* KEY_PRECISION = "precision";
inline const char* KEY_OPT_LEVEL = "opt_level";
inline const char* KEY_DEVICE = "device";
inline const char* KEY_WORKSPACE = "workspace";
inline const char* KEY_STATIC_PLUGIN = "static_plugin";
inline const char* KEY_MAX_DET = "max_det";
inline const char* KEY_CONF = "conf";
inline const char* KEY_IOU = "iou";
inline const char* KEY_NUM_CLASS = "num_class";
inline const char* KEY_CLASSES = "classes";
inline const char* KEY_SOURCE_PATH = "source_path";
inline const char* KEY_ENGINE_PATH = "engine_path";
inline const char* KEY_STATUS = "status";
inline const char* KEY_BUILT_AT = "built_at";
inline const char* KEY_TRT_VERSION = "trt_version";
inline const char* KEY_EVER_BUILT = "ever_built";
inline const char* FIELD_FILE = "file";
inline const char* FIELD_INPUT = "input";
inline const char* FIELD_CONFIG = "config";
inline const char* CONFIG_FILENAME = "config.yaml";
inline const char* MULTIPART_BOUNDARY = "ai_stream2_model_build";
inline const char* MULTIPART_CONTENT_TYPE = "multipart/form-data; boundary=ai_stream2_model_build";

// dirs
inline const char* SOURCE_DIR = "/root/models/source";
inline const char* ENGINE_DIR = "/root/models/engine";
inline const char* SUFFIX_PT = ".pt";
inline const char* SUFFIX_ONNX = ".onnx";
inline const char* SUFFIX_ONNX_DATA = ".onnx.data";
inline const char* SUFFIX_ZIP = ".zip";
inline const char* SUFFIX_ENGINE = ".engine";
inline const char* FILE_LABELS = "labels.txt";
inline const char* FILE_META = "meta.json";
inline const char* UPLOAD_DIR = "/root/tmp";

// types
inline const char* AVAILABLE_TYPES[] = {"YOLO10-DET", "YOLO26-SEG", "YOLO26-DET", "YOLO26-POSE", "STGCNPP", "RTMPOSE"};
inline const char* AVAILABLE_BATCH_MODES[] = {"static", "dynamic"};
inline const char* AVAILABLE_PRECISIONS[] = {"fp16"};
inline const int AVAILABLE_BATCH_RANGE[] = {1, 128};
inline const char* STATUS_NOT_BUILT = "not_built";
inline const char* STATUS_BUILDING = "building";
inline const char* STATUS_BUILT = "built";
inline const char* ERR_BUILDING = "model is building";
inline const char* ERR_IMPORT = "invalid model zip";
inline const char* ERR_NAME = "model name exists";
inline const char* ERR_BUILT = "Partial modification is allowed for a built model";
inline const char* ERR_OCCUPIED = "Model is used by a GIE";
inline const char* SECTION_EXPORT_ONNX = "export_onnx";
inline const char* SECTION_EXPORT_TRT = "export_trt";

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_NAME] = Json::Value();
  node[KEY_TYPE] = Json::Value();
  node[KEY_VERSION] = Json::Value();
  node[KEY_BATCH_SIZE] = 1;
  node[KEY_BATCH_MODE] = "dynamic";
  node[KEY_INPUT_TENSORS] = Json::Value();
  node[KEY_OUTPUT_TENSORS] = Json::Value();
  node[KEY_PRECISION] = "fp16";
  node[KEY_OPT_LEVEL] = Json::Value();
  node[KEY_DEVICE] = 0;
  node[KEY_WORKSPACE] = 8192;
  node[KEY_STATIC_PLUGIN] = Json::Value();
  node[KEY_MAX_DET] = Json::Value();
  node[KEY_CONF] = Json::Value();
  node[KEY_IOU] = Json::Value();
  node[KEY_NUM_CLASS] = Json::Value();
  node[KEY_CLASSES] = Json::Value();
  node[KEY_SOURCE_PATH] = Json::Value();
  node[KEY_STATUS] = STATUS_NOT_BUILT;
  node[KEY_BUILT_AT] = Json::Value();
  node[KEY_TRT_VERSION] = Json::Value();
  node[KEY_EVER_BUILT] = false;
  return node;
}

inline Json::Value BATCH_DATA() {
  Json::Value node;
  node[KEY_UPDATED_ROWS] = Json::Value(Json::arrayValue);
  node[KEY_FAILED_ROWS] = Json::Value(Json::arrayValue);
  node[KEY_SKIPPED_ROWS] = Json::Value(Json::arrayValue);
  node[KEY_UPDATED_COUNT] = Json::UInt64(0);
  node[KEY_FAILED_COUNT] = Json::UInt64(0);
  node[KEY_SKIPPED_COUNT] = Json::UInt64(0);
  return node;
}

// recipes
inline const std::map<std::string, std::map<std::string, std::map<std::string, std::string>>> RECIPES = {
    {"YOLO10-DET", {{"export_onnx", {{"size", "640"}, {"opset", "18"}, {"batch", "null"}, {"dynamic", "true"}, {"simplify", "false"}, {"max_det", "30"}, {"conf", "0.25"}, {"iou", "null"}}}, {"export_trt", {{"batch_size", "18"}, {"gpu_id", "0"}, {"precision", "fp16"}, {"opt_level", "0"}}}}},
    {"YOLO11-DET", {{"export_onnx", {{"size", "640"}, {"opset", "18"}, {"batch", "null"}, {"dynamic", "true"}, {"simplify", "false"}, {"max_det", "30"}, {"conf", "0.25"}, {"iou", "0.45"}}}, {"export_trt", {{"batch_size", "18"}, {"gpu_id", "0"}, {"precision", "fp16"}, {"opt_level", "3"}}}}},
    {"YOLO11-SEG", {{"export_onnx", {{"size", "640"}, {"opset", "18"}, {"batch", "null"}, {"dynamic", "true"}, {"simplify", "false"}, {"max_det", "30"}, {"conf", "0.25"}, {"iou", "0.45"}}}, {"export_trt", {{"batch_size", "18"}, {"gpu_id", "0"}, {"precision", "fp16"}, {"opt_level", "3"}}}}},
    {"YOLO26-DET", {{"export_onnx", {{"size", "640"}, {"opset", "18"}, {"batch", "null"}, {"dynamic", "true"}, {"simplify", "false"}, {"max_det", "30"}, {"conf", "0.25"}, {"iou", "null"}}}, {"export_trt", {{"batch_size", "18"}, {"gpu_id", "0"}, {"precision", "fp16"}, {"opt_level", "3"}}}}},
    {"YOLO26-SEG", {{"export_onnx", {{"size", "640"}, {"opset", "18"}, {"batch", "null"}, {"dynamic", "true"}, {"simplify", "false"}, {"max_det", "30"}, {"conf", "0.25"}, {"iou", "null"}}}, {"export_trt", {{"batch_size", "18"}, {"gpu_id", "0"}, {"precision", "fp16"}, {"opt_level", "3"}}}}},
    {"YOLO26-POSE", {{"export_onnx", {{"size", "640"}, {"opset", "18"}, {"batch", "null"}, {"dynamic", "true"}, {"simplify", "false"}, {"max_det", "30"}, {"conf", "0.25"}, {"iou", "null"}}}, {"export_trt", {{"batch_size", "18"}, {"gpu_id", "0"}, {"precision", "fp16"}, {"opt_level", "3"}}}}},
    {"STGCNPP", {{"export_onnx", {{"size", "null"}, {"opset", "18"}, {"batch", "null"}, {"dynamic", "true"}, {"simplify", "false"}, {"max_det", "null"}, {"conf", "null"}, {"iou", "null"}}}, {"export_trt", {{"batch_size", "64"}, {"gpu_id", "0"}, {"precision", "fp16"}, {"opt_level", "null"}}}}},
    {"RTMPOSE", {{"export_trt", {{"batch_size", "64"}, {"gpu_id", "0"}, {"precision", "fp32"}, {"opt_level", "null"}}}}},
};

// query
inline const char* QUERY_SEARCH = "search";
inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;

// messages
inline const char* MSG_LIST_OK = "Loaded models";
inline const char* MSG_LIST_ERROR = "Failed to load models";
inline const char* MSG_MAP_OK = "Loaded model map";
inline const char* MSG_MAP_ERROR = "Failed to load model map";
inline const char* MSG_CREATE_OK = "Created model";
inline const char* MSG_CREATE_ERROR = "Failed to create model";
inline const char* MSG_GET_OK = "Loaded model";
inline const char* MSG_GET_ERROR = "Failed to load model";
inline const char* MSG_UPDATE_OK = "Updated model";
inline const char* MSG_UPDATE_ERROR = "Failed to update model";
inline const char* MSG_REMOVE_OK = "Removed model";
inline const char* MSG_REMOVE_ERROR = "Failed to remove model";
inline const char* MSG_IMPORT_OK = "Imported models";
inline const char* MSG_IMPORT_ERROR = "Failed to import models";
inline const char* MSG_UPLOAD_OK = "Uploaded model file";
inline const char* MSG_UPLOAD_ERROR = "Failed to upload model file";
inline const char* MSG_BUILD_OK = "Built model";
inline const char* MSG_BUILD_ERROR = "Failed to build model";
inline const char* MSG_STATUS_OK = "Loaded model status";
inline const char* MSG_STATUS_ERROR = "Failed to load model status";

}
