#pragma once

#include <json/json.h>

#include <cstddef>
#include <ctime>
#include <vector>

namespace media {

inline const char* TIME_FORMAT = "%Y-%m-%d %H:%M:%S";
inline const char* TIME_UTC_FORMAT = "%Y-%m-%d %H:%M:%S+00";
inline const char* UPDATED_SEC = "updated_sec";
inline const char* ADMIN_USERNAME = "admin";
inline const std::size_t MD5_CHUNK_SIZE = 1048576;
inline const double BYTES_PER_MB = 1048576.0;
inline const std::time_t RECORDING_SETTLE_SECONDS = 2;

inline const char* KEY_ID = "id";
inline const char* KEY_IDS = "ids";
inline const char* KEY_TYPE = "type";
inline const char* TYPE_IMAGE = "image";
inline const char* TYPE_VIDEO = "video";
inline const char* KEY_NAME = "name";
inline const char* KEY_ITEMS = "items";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_FAILED_COUNT = "failed_count";
inline const char* KEY_SKIPPED_COUNT = "skipped_count";
inline const char* KEY_UPDATED_ROWS = "updated_rows";
inline const char* KEY_FAILED_ROWS = "failed_rows";
inline const char* KEY_SKIPPED_ROWS = "skipped_rows";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";
inline const char* KEY_BASENAME = "basename";
inline const char* KEY_CREATED_AT = "created_at";
inline const char* KEY_UPDATED_AT = "updated_at";
inline const char* KEY_SIZE = "size";
inline const char* KEY_MD5 = "md5";
inline const char* KEY_FOLDER = "folder";
inline const char* MEDIA_DIR = "/root/media";
inline const char* UPLOAD_DIR = "upload";
inline const char* PIPELINE_DIR = "pipeline";
inline const char* RECORDING_DIR = "recording";
inline const char* LOCALE_CTYPE = "C.UTF-8";
inline const char* MEDIAINFO_WIDTH = "Width";
inline const char* MEDIAINFO_HEIGHT = "Height";
inline const char* MEDIAINFO_FPS = "FrameRate";
inline const char* MEDIAINFO_CODEC = "Format";
inline const char* MEDIAINFO_DURATION = "Duration";
inline const std::vector<const char*> SEARCH_COLUMNS{
    "name", "type", "width", "height", "duration", "fps", "size", "updated_at"};
inline const char* KEY_WIDTH = "width";
inline const char* KEY_HEIGHT = "height";
inline const char* KEY_DURATION = "duration";
inline const char* KEY_FPS = "fps";
inline const char* KEY_CODEC = "codec";
inline const char* KEY_USER_ID = "user_id";
inline const char* KEY_STREAM_ID = "stream_id";
inline const char* KEY_PIPELINE_ID = "pipeline_id";
inline const char* ERR_MD5 = "media_err_md5";
inline const char* ERR_ADMIN = "media_err_admin";
inline const char* ERR_SUFFIX = "media_err_suffix";
inline const char* ERR_NOT_FOUND = "media_err_not_found";
inline const char* ERR_ZIP = "media_err_zip";
inline const char* ERR_MEDIAINFO = "media_err_mediainfo";

inline const char* PATH_UPLOAD_FILE = "/ai_stream2/backend/media/upload/file";
inline const char* PATH_PIPELINE_FILE = "/ai_stream2/backend/media/pipeline/file";
inline const char* PATH_RECORDING_FILE = "/ai_stream2/backend/media/recording/file";
inline const char* PATH_UPLOAD_FOLDER = "/ai_stream2/backend/media/upload/folder";
inline const char* PATH_PIPELINE_FOLDER = "/ai_stream2/backend/media/pipeline/folder";
inline const char* PATH_RECORDING_FOLDER = "/ai_stream2/backend/media/recording/folder";
inline const char* PATH_ITEM = "/ai_stream2/backend/media/{media_id}";
inline const char* PATH_CONTENT = "/ai_stream2/backend/media/{media_id}/content";
inline const char* PATH_DOWNLOAD = "/ai_stream2/backend/media/{media_id}/download";
inline const char* PATH_BATCH_UPLOAD = "/ai_stream2/backend/media/batch/upload";
inline const char* PATH_BATCH_CREATE = "/ai_stream2/backend/media/batch/create";
inline const char* PATH_BATCH_REMOVE = "/ai_stream2/backend/media/batch/remove";
inline const char* PATH_BATCH_DOWNLOAD = "/ai_stream2/backend/media/batch/download";
inline const char* DOWNLOAD_PREFIX = "media-";
inline const char* DOWNLOAD_TIME_FORMAT = "%Y%m%d-%H%M%S";
inline const char* DOWNLOAD_SUFFIX = ".zip";

inline const char* MSG_LIST_OK = "media_list_ok";
inline const char* MSG_LIST_ERROR = "media_list_error";
inline const char* MSG_GET_OK = "media_get_ok";
inline const char* MSG_GET_ERROR = "media_get_error";
inline const char* MSG_BATCH_CREATE_OK = "media_batch_create_ok";
inline const char* MSG_BATCH_UPLOAD_OK = "media_batch_upload_ok";
inline const char* MSG_BATCH_UPLOAD_ERROR = "media_batch_upload_error";
inline const char* MSG_BATCH_REMOVE_OK = "media_batch_remove_ok";
inline const char* MSG_BATCH_DOWNLOAD_ERROR = "media_batch_download_error";
inline const char* MSG_DOWNLOAD_ERROR = "media_download_error";

inline const char* IMAGE_SUFFIXES[] = {".jpg", ".jpeg", ".png"};
inline const char* VIDEO_SUFFIXES[] = {".mp4"};

inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const char* QUERY_USER_ID = "user_id";
inline const char* QUERY_STREAM_ID = "stream_id";
inline const char* QUERY_PIPELINE_ID = "pipeline_id";
inline const char* QUERY_TYPE = "type";
inline const char* QUERY_SEARCH = "search";
inline const char* QUERY_DATE = "date";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;

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

inline Json::Value MEDIA_ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_TYPE] = Json::Value();
  node[KEY_NAME] = Json::Value();
  node[KEY_WIDTH] = Json::Value();
  node[KEY_HEIGHT] = Json::Value();
  node[KEY_DURATION] = Json::Value();
  node[KEY_FPS] = Json::Value();
  node[KEY_CODEC] = Json::Value();
  node[KEY_SIZE] = Json::Value();
  node[KEY_MD5] = Json::Value();
  node[KEY_FOLDER] = Json::Value();
  node[KEY_CREATED_AT] = Json::Value();
  node[KEY_UPDATED_AT] = Json::Value();
  node[KEY_USER_ID] = Json::Value();
  node[KEY_STREAM_ID] = Json::Value();
  node[KEY_PIPELINE_ID] = Json::Value();
  return node;
}

}
