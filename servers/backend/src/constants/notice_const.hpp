#pragma once

#include <json/json.h>

namespace notice {

inline const char* KEY_ID = "id";
inline const char* KEY_CREATED_AT = "created_at";
inline const char* KEY_READ_AT = "read_at";
inline const char* KEY_USER_ID = "user_id";
inline const char* KEY_LEVEL = "level";
inline const char* KEY_SOURCE = "source";
inline const char* KEY_TEXT = "text";
inline const char* KEY_REF_ID = "ref_id";
inline const char* KEY_ITEMS = "items";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";
inline const char* ERR_NOT_FOUND = "notice_err_not_found";
inline const char* PATH_LIST = "/ai_stream2/backend/notices";
inline const char* PATH_UNREAD = "/ai_stream2/backend/notices/unread";
inline const char* PATH_READ_AT = "/ai_stream2/backend/notices/{notice_id}/read_at";
inline const char* PATH_ITEM = "/ai_stream2/backend/notices/{notice_id}";
inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;
inline const char* MSG_LIST_OK = "notice_list_ok";
inline const char* MSG_LIST_ERROR = "notice_list_error";
inline const char* MSG_UNREAD_OK = "notice_unread_ok";
inline const char* MSG_UNREAD_ERROR = "notice_unread_error";
inline const char* MSG_READ_AT_OK = "notice_read_at_ok";
inline const char* MSG_READ_AT_ERROR = "notice_read_at_error";
inline const char* MSG_REMOVE_OK = "notice_remove_ok";
inline const char* MSG_REMOVE_ERROR = "notice_remove_error";

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_CREATED_AT] = Json::Value();
  node[KEY_READ_AT] = Json::Value();
  node[KEY_USER_ID] = Json::Value();
  node[KEY_LEVEL] = Json::Value();
  node[KEY_SOURCE] = Json::Value();
  node[KEY_TEXT] = Json::Value();
  node[KEY_REF_ID] = Json::Value();
  return node;
}

}
