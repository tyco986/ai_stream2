#pragma once

#include <json/json.h>

#include <map>
#include <string>
#include <vector>

namespace preview {

inline const char* PATH_LIST = "/ai_stream2/backend/preview";
inline const char* PATH_MAP = "/ai_stream2/backend/preview/map";
inline const char* PATH_TREE = "/ai_stream2/backend/preview/tree";
inline const char* PATH_ACTIVE = "/ai_stream2/backend/preview/active";
inline const char* PATH_ITEM = "/ai_stream2/backend/preview/{preset_id}";

inline const char* MSG_GET_OK = "preview_get_ok";
inline const char* MSG_GET_ERROR = "preview_get_error";
inline const char* MSG_MAP_OK = "preview_map_ok";
inline const char* MSG_MAP_ERROR = "preview_map_error";
inline const char* MSG_TREE_OK = "preview_tree_ok";
inline const char* MSG_TREE_ERROR = "preview_tree_error";
inline const char* MSG_CREATE_OK = "preview_create_ok";
inline const char* MSG_CREATE_ERROR = "preview_create_error";
inline const char* MSG_UPDATE_OK = "preview_update_ok";
inline const char* MSG_UPDATE_ERROR = "preview_update_error";
inline const char* MSG_REMOVE_OK = "preview_remove_ok";
inline const char* MSG_REMOVE_ERROR = "preview_remove_error";
inline const char* MSG_ACTIVE_OK = "preview_active_ok";
inline const char* MSG_ACTIVE_ERROR = "preview_active_error";

inline const char* KEY_ID = "id";
inline const char* KEY_USER_ID = "user_id";
inline const char* KEY_NAME = "name";
inline const char* KEY_LAYOUT = "layout";
inline const char* KEY_MODE = "mode";
inline const char* KEY_SLOTS = "slots";
inline const char* KEY_ACTIVE = "active";
inline const char* KEY_URL = "url";
inline const char* KEY_GROUP_ID = "group_id";
inline const char* KEY_GROUP_NAME = "group_name";
inline const char* KEY_WIDTH = "width";
inline const char* KEY_HEIGHT = "height";
inline const char* KEY_FPS = "fps";
inline const char* KEY_STATUS = "status";
inline const char* KEY_MOUNT = "mount";
inline const char* KEY_RECORD = "record";
inline const char* KEY_PROBED_AT = "probed_at";
inline const char* KEY_TYPE = "type";
inline const char* KEY_CHILDREN = "children";

inline const char* TYPE_GROUP = "group";
inline const char* TYPE_STREAM = "stream";

inline const char* NAME_DEFAULT = "Default";
inline const char* ERR_NOT_FOUND = "preview_err_not_found";
inline const char* ERR_DEFAULT = "preview_err_default";
inline const char* ERR_LAYOUT = "preview_err_layout";
inline const char* ERR_SLOTS = "preview_err_slots";

inline const std::vector<std::string> AVAILABLE_LAYOUT = {"1x1", "2x2", "3x3", "4x4"};

inline const std::map<std::string, int> LAYOUT_SLOT_COUNT = {
    {"1x1", 1},
    {"2x2", 4},
    {"3x3", 9},
    {"4x4", 16},
};

inline const char* SQL_COLUMNS =
    "id::text AS id, user_id::text AS user_id, name, layout, mode, slots::text AS slots, active";

inline const char* SQL_STREAM =
    "stream.id::text AS id, stream.name, stream.group_id::text AS group_id, stream_group.name AS group_name, "
    "stream.url, stream.width, stream.height, stream.fps, stream.status, stream.mount, stream.record, "
    "stream.probed_at ";

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_USER_ID] = Json::Value();
  node[KEY_NAME] = Json::Value();
  node[KEY_LAYOUT] = Json::Value();
  node[KEY_MODE] = Json::Value();
  node[KEY_SLOTS] = Json::Value(Json::objectValue);
  node[KEY_ACTIVE] = false;
  return node;
}

}  // namespace preview
