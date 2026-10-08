#pragma once

namespace stream {

// paths
inline const char* PATH_LIST = "/ai_stream2/backend/streams";
inline const char* PATH_ITEM = "/ai_stream2/backend/streams/{stream_id}";
inline const char* PATH_PROBE = "/ai_stream2/backend/streams/{stream_id}/probe";
inline const char* PATH_BATCH_REMOVE = "/ai_stream2/backend/streams/batch/remove";
inline const char* PATH_BATCH_ENABLE = "/ai_stream2/backend/streams/batch/enable";
inline const char* PATH_BATCH_DISABLE = "/ai_stream2/backend/streams/batch/disable";
inline const char* PATH_BATCH_RECORD = "/ai_stream2/backend/streams/batch/record";
inline const char* PATH_BATCH_UNRECORD = "/ai_stream2/backend/streams/batch/unrecord";
inline const char* PATH_BATCH_PROBE = "/ai_stream2/backend/streams/batch/probe";

// keys
inline const char* KEY_ITEMS = "items";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";
inline const char* KEY_ID = "id";
inline const char* KEY_NAME = "name";
inline const char* KEY_GROUP_ID = "group_id";
inline const char* KEY_GROUP_NAME = "group_name";
inline const char* KEY_URL = "url";
inline const char* KEY_WIDTH = "width";
inline const char* KEY_HEIGHT = "height";
inline const char* KEY_FPS = "fps";
inline const char* KEY_STATUS = "status";
inline const char* KEY_ENABLED = "enabled";
inline const char* KEY_RECORDING = "recording";
inline const char* KEY_PROBED_AT = "probed_at";
inline const char* KEY_IDS = "ids";
inline const char* KEY_REMOVED_COUNT = "removed_count";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_SKIPPED_IDS = "skipped_ids";
inline const char* KEY_RESULTS = "results";

// status
inline const char* STATUS_OFFLINE = "offline";
inline const char* STATUS_ONLINE = "online";

// query
inline const char* QUERY_GROUP_ID = "group_id";
inline const char* QUERY_STREAM_ID = "stream_id";
inline const char* QUERY_SEARCH = "search";
inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;

// messages
inline const char* MSG_LIST_OK = "Loaded streams";
inline const char* MSG_LIST_ERROR = "Failed to load streams";
inline const char* MSG_CREATE_OK = "Created stream";
inline const char* MSG_CREATE_ERROR = "Failed to create stream";
inline const char* MSG_GET_OK = "Loaded stream";
inline const char* MSG_GET_ERROR = "Failed to load stream";
inline const char* MSG_UPDATE_OK = "Updated stream";
inline const char* MSG_UPDATE_ERROR = "Failed to update stream";
inline const char* MSG_DELETE_OK = "Deleted stream";
inline const char* MSG_DELETE_ERROR = "Failed to delete stream";
inline const char* MSG_BATCH_DELETE_OK = "Deleted streams";
inline const char* MSG_BATCH_DELETE_ERROR = "Failed to delete streams";
inline const char* MSG_BATCH_ENABLE_OK = "Enabled streams";
inline const char* MSG_BATCH_ENABLE_ERROR = "Failed to enable streams";
inline const char* MSG_BATCH_DISABLE_OK = "Disabled streams";
inline const char* MSG_BATCH_DISABLE_ERROR = "Failed to disable streams";
inline const char* MSG_BATCH_RECORD_OK = "Recording enabled";
inline const char* MSG_BATCH_RECORD_ERROR = "Failed to enable recording";
inline const char* MSG_BATCH_UNRECORD_OK = "Recording disabled";
inline const char* MSG_BATCH_UNRECORD_ERROR = "Failed to disable recording";
inline const char* MSG_BATCH_PROBE_OK = "Probed streams";
inline const char* MSG_BATCH_PROBE_ERROR = "Failed to probe streams";
inline const char* MSG_PROBE_OK = "Probed stream";
inline const char* MSG_PROBE_ERROR = "Failed to probe stream";

}
