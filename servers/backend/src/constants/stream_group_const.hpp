#pragma once

namespace stream_group {

// paths
inline const char* PATH_TREE = "/ai_stream2/backend/streams/groups/tree";
inline const char* PATH_MAP = "/ai_stream2/backend/streams/groups/map";
inline const char* PATH_IMPORT = "/ai_stream2/backend/streams/groups/import";
inline const char* PATH_EXPORT = "/ai_stream2/backend/streams/groups/export";
inline const char* PATH_GROUP_CREATE = "/ai_stream2/backend/streams/groups/{parent_group_id}";
inline const char* PATH_GROUP = "/ai_stream2/backend/streams/groups/{group_id}";
inline const char* PATH_GROUP_MEMBERS = "/ai_stream2/backend/streams/groups/{group_id}/members";
inline const char* PATH_GROUP_CANDIDATES = "/ai_stream2/backend/streams/groups/{group_id}/candidates";

// keys
inline const char* KEY_ID = "id";
inline const char* KEY_NAME = "name";
inline const char* KEY_TYPE = "type";
inline const char* KEY_CHILDREN = "children";
inline const char* KEY_MOUNT = "mount";
inline const char* KEY_STATUS = "status";
inline const char* KEY_RECORD = "record";
inline const char* KEY_FPS = "fps";
inline const char* KEY_PARENT_ID = "parent_id";
inline const char* KEY_GROUP_ID = "group_id";
inline const char* KEY_GROUP_NAME = "group_name";
inline const char* KEY_STREAM_IDS = "stream_ids";
inline const char* FIELD_FILE = "file";
inline const char* EXPORT_FILENAME = "groups.csv";
inline const char* CSV_HEADER = "group,parent";
inline const char* ALL_GROUP_NAME = "All";
inline const char* MULTIPART_BOUNDARY = "ai_stream2_group_export";
inline const char* MULTIPART_CONTENT_TYPE = "multipart/form-data; boundary=ai_stream2_group_export";
inline const char* ERR_CSV = "stream_group_err_csv";
inline const char* ERR_PARENT_MISSING = "stream_group_err_parent_missing";
inline const char* ERR_GROUP_EXISTS = "stream_group_err_group_exists";
inline const char* ERR_ALL = "stream_group_err_all";
inline const char* ERR_ALL_DELETE = "stream_group_err_all_delete";
inline const char* ALL_GROUP_ID = "00000000-0000-4000-8000-000000000001";

// types
inline const char* TYPE_GROUP = "group";
inline const char* TYPE_STREAM = "stream";

// messages
inline const char* MSG_TREE_OK = "stream_group_tree_ok";
inline const char* MSG_TREE_ERROR = "stream_group_tree_error";
inline const char* MSG_MAP_OK = "stream_group_map_ok";
inline const char* MSG_MAP_ERROR = "stream_group_map_error";
inline const char* MSG_GROUP_CREATE_OK = "stream_group_create_ok";
inline const char* MSG_GROUP_CREATE_ERROR = "stream_group_create_error";
inline const char* MSG_GROUP_UPDATE_OK = "stream_group_update_ok";
inline const char* MSG_GROUP_UPDATE_ERROR = "stream_group_update_error";
inline const char* MSG_GROUP_DELETE_OK = "stream_group_delete_ok";
inline const char* MSG_GROUP_DELETE_ERROR = "stream_group_delete_error";
inline const char* MSG_GROUP_MEMBERS_OK = "stream_group_members_ok";
inline const char* MSG_GROUP_MEMBERS_ERROR = "stream_group_members_error";
inline const char* MSG_GROUP_CANDIDATES_OK = "stream_group_candidates_ok";
inline const char* MSG_GROUP_CANDIDATES_ERROR = "stream_group_candidates_error";
inline const char* MSG_GROUP_SET_MEMBERS_OK = "stream_group_set_members_ok";
inline const char* MSG_GROUP_SET_MEMBERS_ERROR = "stream_group_set_members_error";
inline const char* MSG_IMPORT_OK = "stream_group_import_ok";
inline const char* MSG_IMPORT_ERROR = "stream_group_import_error";
inline const char* MSG_EXPORT_ERROR = "stream_group_export_error";

}
