#pragma once

namespace stream_group {

// paths
inline const char* PATH_TREE = "/ai_stream2/backend/streams/groups/tree";
inline const char* PATH_MAP = "/ai_stream2/backend/streams/groups/map";
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
inline const char* KEY_RECORDING = "recording";
inline const char* KEY_PARENT_ID = "parent_id";
inline const char* KEY_DELETED_GROUP_IDS = "deleted_group_ids";
inline const char* KEY_MOVED_STREAM_COUNT = "moved_stream_count";
inline const char* KEY_STREAM_IDS = "stream_ids";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_REMOVED_COUNT = "removed_count";
inline const char* ALL_GROUP_ID = "00000000-0000-4000-8000-000000000001";

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

}
