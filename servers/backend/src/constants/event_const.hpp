#pragma once

#include <json/json.h>

namespace event {

inline const char* KEY_ID = "id";
inline const char* KEY_OCCURRED_AT = "occurred_at";
inline const char* KEY_STREAM_ID = "stream_id";
inline const char* KEY_STREAM_NAME = "stream_name";
inline const char* KEY_MEDIA_ID = "media_id";
inline const char* KEY_MEDIA_NAME = "media_name";
inline const char* KEY_PIPELINE_ID = "pipeline_id";
inline const char* KEY_PIPELINE_NAME = "pipeline_name";
inline const char* KEY_EVENT_CODE = "event_code";
inline const char* KEY_LEVEL = "level";
inline const char* KEY_EVENT_LABEL = "event_label";
inline const char* LEVEL_ALL = "all";
inline const char* LEVEL_ALERT = "alert";
inline const char* KEY_READ_AT = "read_at";
inline const char* KEY_READER_ID = "reader_id";
inline const char* KEY_READER_NAME = "reader_name";
inline const char* KEY_RAW_PATH = "raw_path";
inline const char* KEY_VISUALIZATION_PATH = "visualization_path";
inline const char* KEY_ANNOTATIONS = "annotations";
inline const char* KEY_ITEMS = "items";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";
inline const char* KEY_IDS = "ids";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_FAILED_COUNT = "failed_count";
inline const char* KEY_UPDATED_ROWS = "updated_rows";
inline const char* KEY_FAILED_ROWS = "failed_rows";
inline const char* ERR_NOT_FOUND = "event_err_not_found";
inline const char* ERR_PIPELINE = "event_err_pipeline";
inline const char* ERR_PAD = "event_err_pad";
inline const char* PATH_LIST = "/backend/events";
inline const char* PATH_WS = "/backend/events/ws";
inline const char* KEY_REFRESH = "refresh";
inline const char* KEY_ACTION = "action";
inline const char* KEY_EVENT = "event";
inline const char* ACTION_INSERT = "insert";
inline const char* ACTION_READ = "read";
inline const char* ACTION_UNREAD = "unread";
inline const char* ACTION_REMOVE = "remove";
inline const char* ACTION_BATCH_READ = "batch_read";
inline const char* ACTION_BATCH_UNREAD = "batch_unread";
inline const char* ACTION_BATCH_REMOVE = "batch_remove";
inline const double REFRESH_QUIET_SECONDS = 0.3;
inline const char* PATH_READ = "/backend/events/{event_id}/read";
inline const char* PATH_UNREAD = "/backend/events/{event_id}/unread";
inline const char* PATH_ITEM = "/backend/events/{event_id}";
inline const char* PATH_CONTENT = "/backend/events/{event_id}/content";
inline const char* PATH_CONTENT_INFO = "/backend/events/{event_id}/content/info";
inline const char* QUERY_KIND = "kind";
inline const char* KIND_RAW = "raw";
inline const char* KIND_VISUALIZATION = "visualization";
inline const char* KEY_WIDTH = "width";
inline const char* KEY_HEIGHT = "height";
inline const char* KEY_SIZE = "size";
inline const char* KEY_CREATED_AT = "created_at";
inline const char* KEY_UPDATED_AT = "updated_at";
inline const double BYTES_PER_MB = 1048576.0;
inline const char* TIME_UTC_FORMAT = "%Y-%m-%dT%H:%M:%SZ";
inline const char* LOCALE_CTYPE = "C.UTF-8";
inline const char* MEDIAINFO_WIDTH = "Width";
inline const char* MEDIAINFO_HEIGHT = "Height";
inline const char* ERR_KIND = "event_err_kind";
inline const char* ERR_CONTENT = "event_err_content";
inline const char* MSG_CONTENT_ERROR = "event_content_error";
inline const char* MSG_CONTENT_INFO_OK = "event_content_info_ok";
inline const char* MSG_CONTENT_INFO_ERROR = "event_content_info_error";
inline const char* PATH_BATCH_READ = "/backend/events/batch/read";
inline const char* PATH_BATCH_UNREAD = "/backend/events/batch/unread";
inline const char* PATH_BATCH_REMOVE = "/backend/events/batch/remove";
inline const char* QUERY_SEARCH = "search";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;
inline const char* MSG_LIST_OK = "event_list_ok";
inline const char* MSG_LIST_ERROR = "event_list_error";
inline const char* MSG_READ_OK = "event_read_ok";
inline const char* MSG_READ_ERROR = "event_read_error";
inline const char* MSG_UNREAD_OK = "event_unread_ok";
inline const char* MSG_UNREAD_ERROR = "event_unread_error";
inline const char* MSG_REMOVE_OK = "event_remove_ok";
inline const char* MSG_REMOVE_ERROR = "event_remove_error";
inline const char* MSG_BATCH_READ_OK = "event_batch_read_ok";
inline const char* MSG_BATCH_UNREAD_OK = "event_batch_unread_ok";
inline const char* MSG_BATCH_REMOVE_OK = "event_batch_remove_ok";

inline Json::Value BATCH_DATA() {
  Json::Value node;
  node[KEY_UPDATED_ROWS] = Json::Value(Json::arrayValue);
  node[KEY_FAILED_ROWS] = Json::Value(Json::arrayValue);
  node[KEY_UPDATED_COUNT] = Json::UInt64(0);
  node[KEY_FAILED_COUNT] = Json::UInt64(0);
  return node;
}

inline Json::Value BANNER_EVENT(const Json::Value& row) {
  Json::Value item;
  item[KEY_ID] = row[KEY_ID];
  item[KEY_OCCURRED_AT] = row[KEY_OCCURRED_AT];
  item[KEY_EVENT_LABEL] = row[KEY_EVENT_LABEL];
  item[KEY_STREAM_NAME] = row[KEY_STREAM_NAME];
  item[KEY_MEDIA_NAME] = row[KEY_MEDIA_NAME];
  item[KEY_PIPELINE_NAME] = row[KEY_PIPELINE_NAME];
  item[KEY_READ_AT] = row[KEY_READ_AT];
  item[KEY_READER_ID] = row[KEY_READER_ID];
  item[KEY_READER_NAME] = row[KEY_READER_NAME];
  return item;
}

inline Json::Value BANNER(const char* action, const Json::Value& event) {
  Json::Value frame;
  frame[KEY_ACTION] = action;
  frame[KEY_EVENT] = event;
  return frame;
}

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_OCCURRED_AT] = Json::Value();
  node[KEY_STREAM_ID] = Json::Value();
  node[KEY_STREAM_NAME] = Json::Value();
  node[KEY_MEDIA_ID] = Json::Value();
  node[KEY_MEDIA_NAME] = Json::Value();
  node[KEY_PIPELINE_ID] = Json::Value();
  node[KEY_PIPELINE_NAME] = Json::Value();
  node[KEY_EVENT_CODE] = Json::Value();
  node[KEY_LEVEL] = Json::Value();
  node[KEY_EVENT_LABEL] = Json::Value();
  node[KEY_READ_AT] = Json::Value();
  node[KEY_READER_ID] = Json::Value();
  node[KEY_READER_NAME] = Json::Value();
  node[KEY_RAW_PATH] = Json::Value();
  node[KEY_VISUALIZATION_PATH] = Json::Value();
  node[KEY_ANNOTATIONS] = Json::Value();
  return node;
}

}
