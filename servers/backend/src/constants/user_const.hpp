#pragma once

#include <json/json.h>

namespace user {

inline const char* PATH_USER_LOGIN = "/ai_stream2/backend/users/login";
inline const char* PATH_USER_LIST = "/ai_stream2/backend/users";
inline const char* PATH_USER_MAP = "/ai_stream2/backend/users/map";
inline const char* PATH_USER_ME = "/ai_stream2/backend/users/me";
inline const char* PATH_USER_ITEM = "/ai_stream2/backend/users/{user_id}";
inline const char* PATH_BATCH_REMOVE = "/ai_stream2/backend/users/batch/remove";
inline const char* PATH_MAP = "/ai_stream2/backend/users/groups/map";
inline const char* PATH_LIST = "/ai_stream2/backend/users/groups";
inline const char* PATH_ITEM = "/ai_stream2/backend/users/groups/{group_id}";
inline const char* PATH_MEMBERS = "/ai_stream2/backend/users/groups/{group_id}/members";
inline const char* PATH_CANDIDATES = "/ai_stream2/backend/users/groups/{group_id}/candidates";

inline const char* QUERY_SEARCH = "search";
inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;

inline const char* MSG_LOGIN_OK = "user_login_ok";
inline const char* MSG_LOGIN_ERROR = "user_login_error";
inline const char* MSG_USER_LIST_OK = "user_list_ok";
inline const char* MSG_USER_LIST_ERROR = "user_list_error";
inline const char* MSG_USER_GET_OK = "user_get_ok";
inline const char* MSG_USER_GET_ERROR = "user_get_error";
inline const char* MSG_USER_MAP_OK = "user_map_ok";
inline const char* MSG_USER_MAP_ERROR = "user_map_error";
inline const char* MSG_USER_CREATE_OK = "user_create_ok";
inline const char* MSG_USER_CREATE_ERROR = "user_create_error";
inline const char* MSG_USER_UPDATE_OK = "user_update_ok";
inline const char* MSG_USER_UPDATE_ERROR = "user_update_error";
inline const char* MSG_USER_REMOVE_OK = "user_remove_ok";
inline const char* MSG_USER_REMOVE_ERROR = "user_remove_error";
inline const char* MSG_BATCH_REMOVE_OK = "user_batch_remove_ok";
inline const char* MSG_LIST_OK = "user_group_list_ok";
inline const char* MSG_LIST_ERROR = "user_group_list_error";
inline const char* MSG_GET_OK = "user_group_get_ok";
inline const char* MSG_GET_ERROR = "user_group_get_error";
inline const char* MSG_MAP_OK = "user_group_map_ok";
inline const char* MSG_MAP_ERROR = "user_group_map_error";
inline const char* MSG_CREATE_OK = "user_group_create_ok";
inline const char* MSG_CREATE_ERROR = "user_group_create_error";
inline const char* MSG_UPDATE_OK = "user_group_update_ok";
inline const char* MSG_UPDATE_ERROR = "user_group_update_error";
inline const char* MSG_REMOVE_OK = "user_group_remove_ok";
inline const char* MSG_REMOVE_ERROR = "user_group_remove_error";
inline const char* MSG_MEMBERS_OK = "user_group_members_ok";
inline const char* MSG_MEMBERS_ERROR = "user_group_members_error";
inline const char* MSG_CANDIDATES_OK = "user_group_candidates_ok";
inline const char* MSG_CANDIDATES_ERROR = "user_group_candidates_error";
inline const char* MSG_SET_MEMBERS_OK = "user_group_set_members_ok";
inline const char* MSG_SET_MEMBERS_ERROR = "user_group_set_members_error";

inline const char* KEY_ITEMS = "items";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";
inline const char* KEY_ID = "id";
inline const char* KEY_NAME = "name";
inline const char* KEY_STREAM_CRUD = "stream_crud";
inline const char* KEY_MODEL_CRUD = "model_crud";
inline const char* KEY_PREVIEW_CRUD = "preview_crud";
inline const char* KEY_PIPELINE_CRUD = "pipeline_crud";
inline const char* KEY_RECORDING_CRUD = "recording_crud";
inline const char* KEY_SERVER_CRUD = "server_crud";
inline const char* KEY_USER_CRUD = "user_crud";
inline const char* KEY_EVENT_CRUD = "event_crud";
inline const char* KEY_USER_IDS = "user_ids";
inline const char* KEY_IDS = "ids";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_FAILED_COUNT = "failed_count";
inline const char* KEY_SKIPPED_COUNT = "skipped_count";
inline const char* KEY_UPDATED_ROWS = "updated_rows";
inline const char* KEY_FAILED_ROWS = "failed_rows";
inline const char* KEY_SKIPPED_ROWS = "skipped_rows";
inline const char* KEY_USERNAME = "username";
inline const char* KEY_PASSWD = "passwd";
inline const char* KEY_TOKEN = "token";
inline const char* ERR_LOGIN = "user_err_login";
inline const char* ERR_UNAUTHORIZED = "user_err_unauthorized";
inline const char* KEY_ACTIVE = "active";
inline const char* KEY_GROUP_ID = "group_id";
inline const char* KEY_GROUP_NAME = "group_name";
inline const char* KEY_LAST_LOGIN = "last_login";
inline const char* ERR_NOT_FOUND = "user_err_not_found";
inline const char* ERR_USER_NOT_FOUND = "user_err_user_not_found";
inline const char* ERR_GROUP_LOCKED = "user_err_group_locked";
inline const char* ERR_USER_LOCKED = "user_err_user_locked";
inline const char* ADMIN_GROUP_ID = "00000000-0000-4000-8000-000000000003";
inline const char* ADMIN_USER_ID = "00000000-0000-4000-8000-000000000004";

inline const char* SQL_USER =
    "\"user\".id::text AS id, \"user\".username, \"user\".passwd, \"user\".active, "
    "\"user\".group_id::text AS group_id, user_group.name AS group_name, \"user\".last_login";
inline const char* SQL_USER_FROM =
    "FROM \"user\" LEFT JOIN user_group ON user_group.id = \"user\".group_id";

inline const char* SQL_ROW =
    "id::text AS id, name, stream_crud, model_crud, preview_crud, pipeline_crud, "
    "recording_crud, server_crud, user_crud, event_crud";

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

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_NAME] = Json::Value();
  node[KEY_STREAM_CRUD] = Json::Value();
  node[KEY_MODEL_CRUD] = Json::Value();
  node[KEY_PREVIEW_CRUD] = Json::Value();
  node[KEY_PIPELINE_CRUD] = Json::Value();
  node[KEY_RECORDING_CRUD] = Json::Value();
  node[KEY_SERVER_CRUD] = Json::Value();
  node[KEY_USER_CRUD] = Json::Value();
  node[KEY_EVENT_CRUD] = Json::Value();
  return node;
}

}
