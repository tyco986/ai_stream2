#pragma once

#include <json/json.h>

namespace pipeline_analyzer {

inline const char* KEY_ID = "id";
inline const char* KEY_NAME = "name";
inline const char* KEY_MEDIA_ID = "media_id";
inline const char* KEY_MEDIA_NAME = "media_name";
inline const char* KEY_WIDTH = "width";
inline const char* KEY_HEIGHT = "height";
inline const char* KEY_ANNOTATIONS = "annotations";
inline const char* KEY_UPDATED_AT = "updated_at";
inline const char* COLUMNS =
    "pipeline_analyzer.id::text AS id, pipeline_analyzer.name, pipeline_analyzer.media_id::text AS media_id, "
    "pipeline_analyzer.annotations::text AS annotations, pipeline_analyzer.updated_at, "
    "media.name AS media_name, media.width, media.height";
inline const char* FROM_MEDIA =
    "FROM pipeline_analyzer LEFT JOIN media ON media.id = pipeline_analyzer.media_id ";
inline const char* SELECT_SAVED =
    "SELECT saved.id::text AS id, saved.name, saved.media_id::text AS media_id, "
    "saved.annotations::text AS annotations, saved.updated_at, "
    "media.name AS media_name, media.width, media.height "
    "FROM saved LEFT JOIN media ON media.id = saved.media_id";
inline const char* ERR_NOT_FOUND = "pipeline_analyzer_err_not_found";
inline const char* ERR_OCCUPIED = "pipeline_analyzer_err_occupied";
inline const char* KEY_IDS = "ids";
inline const char* KEY_UPDATED_ROWS = "updated_rows";
inline const char* KEY_FAILED_ROWS = "failed_rows";
inline const char* KEY_SKIPPED_ROWS = "skipped_rows";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_FAILED_COUNT = "failed_count";
inline const char* KEY_SKIPPED_COUNT = "skipped_count";
inline const char* PATH_LIST = "/backend/pipeline/analyzer";
inline const char* PATH_ITEM = "/backend/pipeline/analyzer/{analyzer_id}";
inline const char* PATH_BATCH_REMOVE = "/backend/pipeline/analyzer/batch/remove";

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_NAME] = Json::Value();
  node[KEY_MEDIA_ID] = Json::Value();
  node[KEY_MEDIA_NAME] = Json::Value();
  node[KEY_WIDTH] = Json::Value();
  node[KEY_HEIGHT] = Json::Value();
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
