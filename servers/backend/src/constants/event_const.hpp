#pragma once

#include <json/json.h>

namespace event {

inline const char* KEY_ID = "id";
inline const char* KEY_OCCURRED_AT = "occurred_at";
inline const char* KEY_STREAM_ID = "stream_id";
inline const char* KEY_STREAM_NAME = "stream_name";
inline const char* KEY_PIPELINE_ID = "pipeline_id";
inline const char* KEY_PIPELINE_NAME = "pipeline_name";
inline const char* KEY_EVENT_CODE = "event_code";
inline const char* KEY_EVENT_LABEL = "event_label";
inline const char* KEY_STATUS = "status";
inline const char* KEY_RAW_PATH = "raw_path";
inline const char* KEY_VISUALIZATION_PATH = "visualization_path";
inline const char* KEY_ANNOTATIONS = "annotations";
inline const char* KEY_ITEMS = "items";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";
inline const char* STATUS_NEW = "new";
inline const char* STATUS_HANDLED = "handled";
inline const char* NOTICE_SOURCE = "event";
inline const char* ERR_NOT_FOUND = "event not found";
inline const char* PATH_LIST = "/ai_stream2/backend/events";
inline const char* PATH_STATUS = "/ai_stream2/backend/events/{event_id}/status";
inline const char* QUERY_SEARCH = "search";
inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;
inline const char* MSG_LIST_OK = "Loaded event list";
inline const char* MSG_LIST_ERROR = "Failed to load event list";
inline const char* MSG_STATUS_OK = "Updated event status";
inline const char* MSG_STATUS_ERROR = "Failed to update event status";

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_OCCURRED_AT] = Json::Value();
  node[KEY_STREAM_ID] = Json::Value();
  node[KEY_STREAM_NAME] = Json::Value();
  node[KEY_PIPELINE_ID] = Json::Value();
  node[KEY_PIPELINE_NAME] = Json::Value();
  node[KEY_EVENT_CODE] = Json::Value();
  node[KEY_EVENT_LABEL] = Json::Value();
  node[KEY_STATUS] = STATUS_NEW;
  node[KEY_RAW_PATH] = Json::Value();
  node[KEY_VISUALIZATION_PATH] = Json::Value();
  node[KEY_ANNOTATIONS] = Json::Value();
  return node;
}

}
