#pragma once

#include <json/json.h>

#include <vector>

namespace stream {

// paths
inline const char* PATH_LIST = "/ai_stream2/backend/streams";
inline const char* PATH_IMPORT = "/ai_stream2/backend/streams/import";
inline const char* PATH_EXPORT = "/ai_stream2/backend/streams/export";
inline const char* PATH_ITEM = "/ai_stream2/backend/streams/{stream_id}";
inline const char* PATH_PROBE = "/ai_stream2/backend/streams/{stream_id}/probe";
inline const char* PATH_BATCH_CREATE = "/ai_stream2/backend/streams/batch/create";
inline const char* PATH_BATCH_REMOVE = "/ai_stream2/backend/streams/batch/remove";
inline const char* PATH_BATCH_MOUNT = "/ai_stream2/backend/streams/batch/mount";
inline const char* PATH_BATCH_UNMOUNT = "/ai_stream2/backend/streams/batch/unmount";
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
inline const char* KEY_MOUNT = "mount";
inline const char* KEY_RECORD = "record";
inline const char* KEY_PROBED_AT = "probed_at";
inline const char* FIELD_FILE = "file";
inline const char* EXPORT_FILENAME = "streams.csv";
inline const char* MULTIPART_BOUNDARY = "ai_stream2_stream_export";
inline const char* MULTIPART_CONTENT_TYPE = "multipart/form-data; boundary=ai_stream2_stream_export";
inline const char* KEY_IDS = "ids";
inline const char* KEY_REMOVED_COUNT = "removed_count";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_FAILED_COUNT = "failed_count";
inline const char* KEY_SKIPPED_COUNT = "skipped_count";
inline const char* KEY_UPDATED_ROWS = "updated_rows";
inline const char* KEY_FAILED_ROWS = "failed_rows";
inline const char* KEY_SKIPPED_ROWS = "skipped_rows";
inline const char* KEY_SKIPPED_IDS = "skipped_ids";
inline const char* KEY_RESULTS = "results";

// status
inline const char* STATUS_OFFLINE = "offline";
inline const char* STATUS_ONLINE = "online";

inline const char* ALL_GROUP_ID = "00000000-0000-4000-8000-000000000001";

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_NAME] = Json::Value();
  node[KEY_GROUP_ID] = ALL_GROUP_ID;
  node[KEY_URL] = Json::Value();
  node[KEY_WIDTH] = Json::Value();
  node[KEY_HEIGHT] = Json::Value();
  node[KEY_FPS] = Json::Value();
  node[KEY_STATUS] = STATUS_OFFLINE;
  node[KEY_MOUNT] = true;
  node[KEY_RECORD] = false;
  node[KEY_PROBED_AT] = Json::Value();
  return node;
}

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

// query
inline const char* QUERY_GROUP_ID = "group_id";
inline const char* QUERY_STREAM_ID = "stream_id";
inline const char* QUERY_SEARCH = "search";
inline const std::vector<const char*> SEARCH_COLUMNS{
    "stream.name",
    "stream_group.name",
    "stream.width",
    "stream.height",
    "stream.fps",
    "stream.status",
    "stream.probed_at",
    "stream.mount",
    "stream.record"};
inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;

// messages
inline const char* MSG_LIST_OK = "stream_list_ok";
inline const char* MSG_LIST_ERROR = "stream_list_error";
inline const char* MSG_IMPORT_OK = "stream_import_ok";
inline const char* MSG_IMPORT_ERROR = "stream_import_error";
inline const char* MSG_EXPORT_ERROR = "stream_export_error";
inline const char* MSG_CREATE_OK = "stream_create_ok";
inline const char* MSG_CREATE_ERROR = "stream_create_error";
inline const char* MSG_GET_OK = "stream_get_ok";
inline const char* MSG_GET_ERROR = "stream_get_error";
inline const char* MSG_UPDATE_OK = "stream_update_ok";
inline const char* MSG_UPDATE_ERROR = "stream_update_error";
inline const char* MSG_DELETE_OK = "stream_delete_ok";
inline const char* MSG_DELETE_ERROR = "stream_delete_error";
inline const char* MSG_BATCH_CREATE_OK = "stream_batch_create_ok";
inline const char* MSG_BATCH_CREATE_PARTIAL = "stream_batch_create_partial";
inline const char* MSG_BATCH_CREATE_REJECTED = "stream_batch_create_rejected";
inline const char* MSG_BATCH_CREATE_ERROR = "stream_batch_create_error";
inline const char* MSG_BATCH_DELETE_OK = "stream_batch_delete_ok";
inline const char* MSG_BATCH_DELETE_ERROR = "stream_batch_delete_error";
inline const char* MSG_BATCH_MOUNT_OK = "stream_batch_mount_ok";
inline const char* MSG_BATCH_MOUNT_ERROR = "stream_batch_mount_error";
inline const char* MSG_BATCH_UNMOUNT_OK = "stream_batch_unmount_ok";
inline const char* MSG_BATCH_UNMOUNT_ERROR = "stream_batch_unmount_error";
inline const char* MSG_BATCH_RECORD_OK = "stream_batch_record_ok";
inline const char* MSG_BATCH_RECORD_ERROR = "stream_batch_record_error";
inline const char* MSG_BATCH_UNRECORD_OK = "stream_batch_unrecord_ok";
inline const char* MSG_BATCH_UNRECORD_ERROR = "stream_batch_unrecord_error";
inline const char* MSG_BATCH_PROBE_OK = "stream_batch_probe_ok";
inline const char* MSG_BATCH_PROBE_ERROR = "stream_batch_probe_error";
inline const char* CSV_HEADER = "name,url,group";
inline const char* ERR_CSV = "stream_err_csv";
inline const char* ERR_GROUP_MISSING = "stream_err_group_missing";
inline const char* ERR_NAME_EXISTS = "stream_err_name_exists";
inline const char* ERR_URL_EXISTS = "stream_err_url_exists";
inline const char* MSG_PROBE_OK = "stream_probe_ok";
inline const char* MSG_PROBE_ERROR = "stream_probe_error";

}
