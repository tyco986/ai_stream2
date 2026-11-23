#pragma once

#include <json/json.h>

namespace notice {

inline const char* KEY_ID = "id";
inline const char* KEY_CREATED_AT = "created_at";
inline const char* KEY_READ_AT = "read_at";
inline const char* KEY_READ_BY = "read_by";
inline const char* KEY_LEVEL = "level";
inline const char* KEY_SOURCE = "source";
inline const char* KEY_TEXT = "text";
inline const char* KEY_REF_ID = "ref_id";
inline const char* KEY_ITEMS = "items";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_EVENT = "event";
inline const char* KEY_SYSTEM = "system";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";
inline const char* LEVEL_SYSTEM = "system";
inline const char* ERR_NOT_FOUND = "notice_err_not_found";
inline const char* ERR_LEVEL = "notice_err_level";
inline const char* PATH_LIST = "/backend/notices";
inline const char* PATH_COUNT = "/backend/notices/count";
inline const char* PATH_READ = "/backend/notices/{notice_id}/read";
inline const char* PATH_UNREAD = "/backend/notices/{notice_id}/unread";
inline const char* PATH_ITEM = "/backend/notices/{notice_id}";
inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const char* QUERY_LEVEL = "level";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;
inline const char* MSG_CREATE_OK = "notice_create_ok";
inline const char* MSG_CREATE_ERROR = "notice_create_error";
inline const char* MSG_LIST_OK = "notice_list_ok";
inline const char* MSG_LIST_ERROR = "notice_list_error";
inline const char* MSG_COUNT_OK = "notice_count_ok";
inline const char* MSG_COUNT_ERROR = "notice_count_error";
inline const char* MSG_READ_OK = "notice_read_ok";
inline const char* MSG_READ_ERROR = "notice_read_error";
inline const char* MSG_UNREAD_OK = "notice_unread_ok";
inline const char* MSG_UNREAD_ERROR = "notice_unread_error";
inline const char* MSG_REMOVE_OK = "notice_remove_ok";
inline const char* MSG_REMOVE_ERROR = "notice_remove_error";

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_CREATED_AT] = Json::Value();
  node[KEY_READ_AT] = Json::Value();
  node[KEY_READ_BY] = Json::Value();
  node[KEY_LEVEL] = Json::Value();
  node[KEY_SOURCE] = Json::Value();
  node[KEY_TEXT] = Json::Value();
  node[KEY_REF_ID] = Json::Value();
  return node;
}

}
