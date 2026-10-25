#pragma once

#include <json/json.h>

namespace pipeline_analyzer {

inline const char* KEY_ID = "id";
inline const char* KEY_NAME = "name";
inline const char* KEY_SOURCE_PATH = "source_path";
inline const char* KEY_ANNOTATIONS = "annotations";
inline const char* KEY_UPDATED_AT = "updated_at";

inline const char* SOURCE_DIR = "/root/analyzers";
inline const char* SUFFIX_JPG = ".jpg";
inline const char* SUFFIX_JPEG = ".jpeg";
inline const char* SUFFIX_PNG = ".png";
inline const char* ERR_NOT_FOUND = "pipeline_analyzer_err_not_found";
inline const char* ERR_OCCUPIED = "pipeline_analyzer_err_occupied";
inline const char* KEY_IDS = "ids";
inline const char* KEY_UPDATED_ROWS = "updated_rows";
inline const char* KEY_FAILED_ROWS = "failed_rows";
inline const char* KEY_SKIPPED_ROWS = "skipped_rows";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_FAILED_COUNT = "failed_count";
inline const char* KEY_SKIPPED_COUNT = "skipped_count";
inline const char* FIELD_FILE = "file";
inline const char* PATH_LIST = "/ai_stream2/backend/pipeline/analyzer";
inline const char* PATH_ITEM = "/ai_stream2/backend/pipeline/analyzer/{analyzer_id}";
inline const char* PATH_BATCH_REMOVE = "/ai_stream2/backend/pipeline/analyzer/batch/remove";

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_NAME] = Json::Value();
  node[KEY_SOURCE_PATH] = Json::Value();
  node[KEY_ANNOTATIONS] = Json::Value(Json::objectValue);
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

inline const char* KEY_ITEMS = "items";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";
inline const char* QUERY_SEARCH = "search";
inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;
inline const char* MSG_LIST_OK = "pipeline_analyzer_list_ok";
inline const char* MSG_LIST_ERROR = "pipeline_analyzer_list_error";
inline const char* MSG_GET_OK = "pipeline_analyzer_get_ok";
inline const char* MSG_GET_ERROR = "pipeline_analyzer_get_error";
inline const char* MSG_CREATE_OK = "pipeline_analyzer_create_ok";
inline const char* MSG_CREATE_ERROR = "pipeline_analyzer_create_error";
inline const char* MSG_UPDATE_OK = "pipeline_analyzer_update_ok";
inline const char* MSG_UPDATE_ERROR = "pipeline_analyzer_update_error";
inline const char* MSG_REMOVE_OK = "pipeline_analyzer_remove_ok";
inline const char* MSG_REMOVE_ERROR = "pipeline_analyzer_remove_error";

}
