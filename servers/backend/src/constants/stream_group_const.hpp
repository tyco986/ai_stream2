#pragma once

#include <json/json.h>

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
inline const char* KEY_ENABLED = "enabled";
inline const char* KEY_STATUS = "status";
inline const char* KEY_RECORD = "record";
inline const char* KEY_PARENT_ID = "parent_id";
inline const char* KEY_STREAM_IDS = "stream_ids";
inline const char* FIELD_FILE = "file";
inline const char* EXPORT_FILENAME = "groups.csv";
inline const char* CSV_HEADER = "group,parent";
inline const char* ALL_GROUP_NAME = "All";
inline const char* MULTIPART_BOUNDARY = "ai_stream2_group_export";
inline const char* MULTIPART_CONTENT_TYPE = "multipart/form-data; boundary=ai_stream2_group_export";
inline const char* ERR_CSV = "invalid csv";
inline const char* ERR_PARENT_MISSING = "parent not found";
inline const char* ERR_GROUP_EXISTS = "group name exists";
inline const char* ERR_ALL = "cannot move All";
inline const char* ALL_GROUP_ID = "00000000-0000-4000-8000-000000000001";

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_NAME] = Json::Value();
  node[KEY_PARENT_ID] = Json::Value();
  return node;
}

// types
inline const char* TYPE_GROUP = "group";
inline const char* TYPE_STREAM = "stream";

// messages
inline const char* MSG_TREE_OK = "Loaded group tree";
inline const char* MSG_TREE_ERROR = "Failed to load group tree";
inline const char* MSG_MAP_OK = "Loaded group map";
inline const char* MSG_MAP_ERROR = "Failed to load group map";
inline const char* MSG_GROUP_CREATE_OK = "Created group";
inline const char* MSG_GROUP_CREATE_ERROR = "Failed to create group";
inline const char* MSG_GROUP_RENAME_OK = "Renamed group";
inline const char* MSG_GROUP_RENAME_ERROR = "Failed to rename group";
inline const char* MSG_GROUP_DELETE_OK = "Deleted group";
inline const char* MSG_GROUP_DELETE_ERROR = "Failed to delete group";
inline const char* MSG_GROUP_MEMBERS_OK = "Loaded group members";
inline const char* MSG_GROUP_MEMBERS_ERROR = "Failed to load group members";
inline const char* MSG_GROUP_CANDIDATES_OK = "Loaded group candidates";
inline const char* MSG_GROUP_CANDIDATES_ERROR = "Failed to load group candidates";
inline const char* MSG_GROUP_SET_MEMBERS_OK = "Updated group members";
inline const char* MSG_GROUP_SET_MEMBERS_ERROR = "Failed to update group members";
inline const char* MSG_IMPORT_OK = "Imported groups";
inline const char* MSG_IMPORT_ERROR = "Failed to import groups";
inline const char* MSG_EXPORT_ERROR = "Failed to export groups";

}
