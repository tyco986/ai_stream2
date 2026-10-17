#pragma once

#include <json/json.h>

namespace pipeline_gie {

inline const char* KEY_ID = "id";
inline const char* KEY_NAME = "name";
inline const char* KEY_MODEL_ID = "model_id";
inline const char* KEY_MODEL_NAME = "model_name";
inline const char* KEY_CLASS_ATTRS = "class_attrs";
inline const char* KEY_INTERVAL = "interval";
inline const char* KEY_ENGINE_PATH = "engine_path";
inline const char* KEY_UPDATED_AT = "updated_at";
inline const char* KEY_STATUS = "status";
inline const char* KEY_IDS = "ids";
inline const char* KEY_UPDATED_ROWS = "updated_rows";
inline const char* KEY_FAILED_ROWS = "failed_rows";
inline const char* KEY_SKIPPED_ROWS = "skipped_rows";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_FAILED_COUNT = "failed_count";
inline const char* KEY_SKIPPED_COUNT = "skipped_count";
inline const char* PATH_LIST = "/ai_stream2/backend/pipeline/gie";
inline const char* PATH_MAP = "/ai_stream2/backend/pipeline/gie/map";
inline const char* PATH_MODELS = "/ai_stream2/backend/pipeline/gie/models";
inline const char* PATH_BATCH_REMOVE = "/ai_stream2/backend/pipeline/gie/batch/remove";
inline const char* PATH_ITEM = "/ai_stream2/backend/pipeline/gie/{gie_id}";
inline const char* ERR_NOT_FOUND = "gie not found";
inline const char* ERR_OCCUPIED = "gie is used by a pipeline";

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_NAME] = Json::Value();
  node[KEY_MODEL_ID] = Json::Value();
  node[KEY_MODEL_NAME] = Json::Value();
  node[KEY_CLASS_ATTRS] = Json::Value();
  node[KEY_INTERVAL] = Json::Value();
  node[KEY_ENGINE_PATH] = Json::Value();
  node[KEY_UPDATED_AT] = Json::Value();
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

inline const char* MSG_MODELS_OK = "Loaded models";
inline const char* MSG_MODELS_ERROR = "Failed to load models";
inline const char* MSG_MAP_OK = "Loaded gie map";
inline const char* MSG_MAP_ERROR = "Failed to load gie map";
inline const char* MSG_GET_OK = "Loaded gie";
inline const char* MSG_GET_ERROR = "Failed to load gie";
inline const char* MSG_CREATE_OK = "Created gie";
inline const char* MSG_CREATE_ERROR = "Failed to create gie";
inline const char* MSG_UPDATE_OK = "Updated gie";
inline const char* MSG_UPDATE_ERROR = "Failed to update gie";
inline const char* MSG_REMOVE_OK = "Removed gie";
inline const char* MSG_REMOVE_ERROR = "Failed to remove gie";

}
