#pragma once

#include <cstddef>

#include <json/json.h>

namespace pipeline {

inline const char* PATH_LIST = "/backend/pipelines";
inline const char* PATH_MAP = "/backend/pipelines/map";
inline const char* PATH_ITEM = "/backend/pipelines/{pipeline_id}";
inline const char* PATH_START = "/backend/pipelines/{pipeline_id}/start";
inline const char* PATH_PROBE = "/backend/pipelines/{pipeline_id}/probe";
inline const char* PATH_STOP = "/backend/pipelines/{pipeline_id}/stop";
inline const char* PATH_LOGS_API = "/backend/pipelines/{pipeline_id}/logs/api";
inline const char* PATH_LOGS_PAD = "/backend/pipelines/{pipeline_id}/logs/pad";
inline const char* PATH_LOGS_GSTREAMER = "/backend/pipelines/{pipeline_id}/logs/gstreamer";
inline const char* PATH_LOGS = "/backend/pipelines/{pipeline_id}/logs";
inline const char* PATH_BATCH_STOP = "/backend/pipelines/batch/stop";
inline const char* PATH_BATCH_REMOVE = "/backend/pipelines/batch/remove";
inline const char* PATH_GENERATE = "/ai_stream2/generator/generate";

inline const char* KEY_ID = "id";
inline const char* KEY_NAME = "name";
inline const char* KEY_TYPE = "type";
inline const char* KEY_STATUS = "status";
inline const char* KEY_INPUT = "input";
inline const char* KEY_CONFIG_PATH = "config_path";
inline const char* KEY_GIE_ID = "gie_id";
inline const char* KEY_ANALYZER = "analyzer";
inline const char* KEY_ANALYZER_ID = "analyzer_id";
inline const char* KEY_STREAM_IDS = "stream_ids";
inline const char* KEY_STREAM_NAMES = "stream_names";
inline const char* KEY_MEDIA_IDS = "media_ids";
inline const char* KEY_MEDIA_NAMES = "media_names";
inline const char* KEY_GIE_IDS = "gie_ids";
inline const char* KEY_GIE_NAMES = "gie_names";
inline const char* KEY_ANALYZER_IDS = "analyzer_ids";
inline const char* KEY_ANALYZER_NAMES = "analyzer_names";
inline const char* KEY_PIPELINE_NAME = "pipeline_name";
inline const char* KEY_GENERATOR = "generator";
inline const char* KEY_START_TIME = "start_time";
inline const char* KEY_END_TIME = "end_time";
inline const char* KEY_DURATION = "duration";
inline const char* STEP_GENERATE = "generate";
inline const char* STEP_INSPECT = "inspect";
inline const char* STEP_RUNNING = "running";
inline const char* STEP_CREATE = "create";
inline const char* STEP_RESTART = "restart";
inline const char* STEP_HEALTH = "health";
inline const char* STEP_PACK = "pack";
inline const char* STEP_POST = "post";
inline const char* STEP_STATUS = "status";
inline const char* STEP_STOP = "stop";
inline const char* STEP_DELETE = "delete";
inline const char* KEY_UPDATED_AT = "updated_at";
inline const char* KEY_ITEMS = "items";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";
inline const char* KEY_IDS = "ids";
inline const char* KEY_UPDATED_ROWS = "updated_rows";
inline const char* KEY_FAILED_ROWS = "failed_rows";
inline const char* KEY_SKIPPED_ROWS = "skipped_rows";
inline const char* KEY_UPDATED_COUNT = "updated_count";
inline const char* KEY_FAILED_COUNT = "failed_count";
inline const char* KEY_SKIPPED_COUNT = "skipped_count";

inline const char* FIELD_INPUT = "input";
inline const char* FIELD_CONFIG = "config";
inline const char* FIELD_MODEL = "model";
inline const char* FIELD_MEDIA = "media";
inline const char* FIELD_FILE = "file";
inline const char* GENERATOR_BOUNDARY = "ai_stream2_generator";
inline const char* MEDIA_FILENAME = "media.zip";
inline const char* KEY_MODEL_ZIP = "model_zip";
inline const char* KEY_VIDEOS = "videos";
inline const char* KEY_IMAGES = "images";
inline const char* KEY_STREAMS = "streams";
inline const char* KEY_URL = "url";
inline const char* KEY_WIDTH = "width";
inline const char* KEY_HEIGHT = "height";
inline const char* KEY_FPS = "fps";
inline const char* KEY_PATH = "path";
inline const char* KEY_PGIE = "pgie";
inline const char* KEY_SGIE = "sgie";
inline const char* KEY_RTMPOSE = "rtmpose";
inline const char* KEY_STGCNPP = "stgcnpp";
inline const char* ANALYZER_STREAM_SUFFIX = "-stream";
inline const char* KEY_EVENT_CODER = "event_coder";
inline const char* KEY_COLLECTOR = "collector";
inline const char* KEY_FILESINK_OUTPUT = "filesink_output";
inline const char* KEY_CAPTURER_OUTPUT = "capturer_output";
inline const char* FILESINK_DIR = "/root/media/pipeline";
inline const char* VIS_GENERATOR_MARK = "Vis";
inline const char* VIS_RTSP_MARK = "VisRTSP";
inline const int OUTPUT_SUFFIX_LIMIT = 1000;
inline const char* PARAMS_YML = "params.yml";
inline const char* YAML_STRING_TAG = "tag:yaml.org,2002:str";
inline const char* MULTIPART_BOUNDARY = "ai_stream2_pipeline_generate";
inline const char* MULTIPART_CONTENT_TYPE = "multipart/form-data; boundary=ai_stream2_pipeline_generate";

inline const char* CONFIG_DIR = "/root/configs";
inline const char* SUFFIX_ZIP = ".zip";
inline const char* STATUS_STOPPED = "stopped";
inline const char* STATUS_STARTING = "starting";
inline const char* STATUS_RUNNING = "running";
inline const char* ERR_BUSY = "pipeline_err_busy";
inline const char* ERR_LOGS_BUSY = "stop the pipeline before clearing logs";
inline const char* ERR_IDLE = "pipeline is not running";

inline const char* DOCKER_BASE_URL = "http://ai_stream2_docker_socket_proxy:2375";
inline const char* DOCKER_CREATE_PATH = "/containers/create";
inline const char* DOCKER_STOP_PATH = "/stop";
inline const char* DOCKER_RESTART_PATH = "/restart";
inline const char* DOCKER_JSON_PATH = "/json";
inline const char* DOCKER_NETWORK = "ai_stream2";
inline const char* DOCKER_RUNTIME = "nvidia";
inline const char* LOGS_DIR = "/root/logs";
inline const char* DEEPSTREAM_LOG_DIR = "/root/logs/deepstream";
inline const char* LOG_API_DIR = "api";
inline const char* LOG_PAD_DIR = "pad";
inline const int PAD_WIDTH = 3;
inline const char* LOG_API_PAGE = "deepstream";
inline const char* LOG_API_SUFFIX = ".jsonl";
inline const char* LOG_GSTREAMER_DIR = "gstreamer";
inline const char* LOG_GSTREAMER_PREFIX = "gstreamer_";
inline const char* LOG_GSTREAMER_SUFFIX = ".log";
inline const char* KEY_CONTENT = "content";
inline const char* KEY_LATENCY = "latency";
inline const std::size_t DATE_LENGTH = 10;
inline const int LATENCY_DIGITS = 2;
inline const char* MEDIA_DIR = "/root/media";
inline const char* EVENT_DIR = "/root/events";
inline const char* LOCALTIME_BIND = "/etc/localtime:/etc/localtime:ro";
inline const char* MOUNTINFO_PATH = "/proc/self/mountinfo";
inline const char* DOCKER_GPU_DRIVER = "nvidia";
inline const char* DOCKER_GPU_CAPABILITY = "gpu";
inline const int DOCKER_GPU_COUNT = -1;
inline const char* DEEPSTREAM_IMAGE = "ai_stream2_deepstream";
inline const char* DEEPSTREAM_BIN = "deepstream";
inline const char* CONTAINER_PREFIX = "ai_stream2_";
inline const int DEEPSTREAM_PORT = 8094;
inline const char* DEEPSTREAM_HEALTH_PATH = "/ai_stream2/deepstream/health";
inline const char* DEEPSTREAM_START_PATH = "/start";
inline const char* DEEPSTREAM_STOP_PATH = "/stop";
inline const char* DEEPSTREAM_STATUS_PATH = "/status";
inline const char* DEEPSTREAM_LOGS_PATH = "/ai_stream2/deepstream/logs";
inline const int HEALTH_ATTEMPTS = 15;
inline const int STATUS_ATTEMPTS = 15;
inline const double HEALTH_INTERVAL_SECONDS = 1.0;
inline const double GENERATE_TIMEOUT_SECONDS = 3.0;
inline const double DOCKER_TIMEOUT_SECONDS = 20.0;
inline const double HEALTH_TIMEOUT_SECONDS = 3.0;
inline const double START_TIMEOUT_SECONDS = 5.0;
inline const double STATUS_TIMEOUT_SECONDS = 3.0;

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_NAME] = Json::Value();
  node[KEY_TYPE] = Json::Value();
  node[KEY_STATUS] = STATUS_STOPPED;
  node[KEY_INPUT] = Json::Value();
  node[KEY_CONFIG_PATH] = Json::Value();
  node[KEY_STREAM_IDS] = Json::Value(Json::arrayValue);
  node[KEY_MEDIA_IDS] = Json::Value(Json::arrayValue);
  node[KEY_GIE_IDS] = Json::Value(Json::arrayValue);
  node[KEY_ANALYZER_IDS] = Json::Value(Json::arrayValue);
  node[KEY_START_TIME] = Json::Value();
  node[KEY_END_TIME] = Json::Value();
  node[KEY_DURATION] = Json::Value();
  node[KEY_UPDATED_AT] = Json::Value();
  return node;
}

inline const char* MSG_LIST_OK = "pipeline_list_ok";
inline const char* MSG_LIST_ERROR = "pipeline_list_error";
inline const char* MSG_MAP_OK = "pipeline_map_ok";
inline const char* MSG_MAP_ERROR = "pipeline_map_error";
inline const char* MSG_GET_OK = "pipeline_get_ok";
inline const char* MSG_GET_ERROR = "pipeline_get_error";
inline const char* MSG_CREATE_OK = "pipeline_create_ok";
inline const char* MSG_CREATE_ERROR = "pipeline_create_error";
inline const char* MSG_UPDATE_OK = "pipeline_update_ok";
inline const char* MSG_UPDATE_ERROR = "pipeline_update_error";
inline const char* MSG_REMOVE_OK = "pipeline_remove_ok";
inline const char* MSG_REMOVE_ERROR = "pipeline_remove_error";
inline const char* MSG_START_OK = "pipeline_start_ok";
inline const char* MSG_START_ERROR = "pipeline_start_error";
inline const char* MSG_STOP_OK = "pipeline_stop_ok";
inline const char* MSG_STOP_ERROR = "pipeline_stop_error";
inline const char* MSG_BATCH_STOP_OK = "pipeline_batch_stop_ok";
inline const char* MSG_BATCH_REMOVE_OK = "pipeline_batch_remove_ok";
inline const char* MSG_PROBE_OK = "pipeline_probe_ok";
inline const char* MSG_PROBE_ERROR = "pipeline_probe_error";
inline const char* MSG_LOGS_OK = "pipeline_logs_ok";
inline const char* MSG_LOGS_ERROR = "pipeline_logs_error";

inline const char* SQL_GIE_USED =
    "input->>'pgie' = $1 OR input->>'sgie' = $1 OR input->>'rtmpose' = $1 OR input->>'stgcnpp' = $1";
inline const char* SQL_ANALYZER_USED =
    "EXISTS (SELECT 1 FROM jsonb_path_query(input, '$.streams[*].analyzer') AS slot(body) WHERE slot.body #>> '{}' = $1) "
    "OR EXISTS (SELECT 1 FROM jsonb_path_query(input, '$.videos[*].analyzer') AS slot(body) WHERE slot.body #>> '{}' = $1) "
    "OR EXISTS (SELECT 1 FROM jsonb_path_query(input, '$.images[*].analyzer') AS slot(body) WHERE slot.body #>> '{}' = $1)";
inline const char* SQL_REF_COLUMNS =
    "COALESCE((SELECT json_agg(item.id ORDER BY item.ord) FROM ("
    "SELECT elem->>'id' AS id, ord FROM jsonb_array_elements(COALESCE(input->'streams', '[]'::jsonb)) WITH ORDINALITY AS src(elem, ord)) item "
    "WHERE item.id IS NOT NULL), '[]'::json)::text AS stream_ids, "
    "COALESCE((SELECT json_agg(stream.name ORDER BY item.ord) FROM ("
    "SELECT elem->>'id' AS id, ord FROM jsonb_array_elements(COALESCE(input->'streams', '[]'::jsonb)) WITH ORDINALITY AS src(elem, ord)) item "
    "LEFT JOIN stream ON stream.id::text = item.id WHERE item.id IS NOT NULL), '[]'::json)::text AS stream_names, "
    "COALESCE((SELECT json_agg(item.id ORDER BY item.ord) FROM ("
    "SELECT elem->>'id' AS id, ord FROM jsonb_array_elements(COALESCE(input->'videos', '[]'::jsonb)) WITH ORDINALITY AS src(elem, ord) "
    "UNION ALL SELECT elem->>'id', ord FROM jsonb_array_elements(COALESCE(input->'images', '[]'::jsonb)) WITH ORDINALITY AS src(elem, ord)) item "
    "WHERE item.id IS NOT NULL), '[]'::json)::text AS media_ids, "
    "COALESCE((SELECT json_agg(media.name ORDER BY item.ord) FROM ("
    "SELECT elem->>'id' AS id, ord FROM jsonb_array_elements(COALESCE(input->'videos', '[]'::jsonb)) WITH ORDINALITY AS src(elem, ord) "
    "UNION ALL SELECT elem->>'id', ord FROM jsonb_array_elements(COALESCE(input->'images', '[]'::jsonb)) WITH ORDINALITY AS src(elem, ord)) item "
    "LEFT JOIN media ON media.id::text = item.id WHERE item.id IS NOT NULL), '[]'::json)::text AS media_names, "
    "COALESCE((SELECT json_agg(item.id ORDER BY item.ord) FROM ("
    "SELECT 1 AS ord, input->>'pgie' AS id UNION ALL SELECT 2, input->>'sgie' "
    "UNION ALL SELECT 3, input->>'rtmpose' UNION ALL SELECT 4, input->>'stgcnpp') item WHERE item.id IS NOT NULL), '[]'::json)::text AS gie_ids, "
    "COALESCE((SELECT json_agg(pipeline_gie.name ORDER BY item.ord) FROM ("
    "SELECT 1 AS ord, input->>'pgie' AS id UNION ALL SELECT 2, input->>'sgie' "
    "UNION ALL SELECT 3, input->>'rtmpose' UNION ALL SELECT 4, input->>'stgcnpp') item "
    "LEFT JOIN pipeline_gie ON pipeline_gie.id::text = item.id WHERE item.id IS NOT NULL), '[]'::json)::text AS gie_names, "
    "COALESCE((SELECT json_agg(item.id) FROM ("
    "SELECT slot.body #>> '{}' AS id FROM jsonb_path_query(input, '$.streams[*].analyzer') AS slot(body) "
    "UNION ALL SELECT slot.body #>> '{}' FROM jsonb_path_query(input, '$.videos[*].analyzer') AS slot(body) "
    "UNION ALL SELECT slot.body #>> '{}' FROM jsonb_path_query(input, '$.images[*].analyzer') AS slot(body)) item "
    "WHERE item.id IS NOT NULL), '[]'::json)::text AS analyzer_ids, "
    "COALESCE((SELECT json_agg(pipeline_analyzer.name) FROM ("
    "SELECT slot.body #>> '{}' AS id FROM jsonb_path_query(input, '$.streams[*].analyzer') AS slot(body) "
    "UNION ALL SELECT slot.body #>> '{}' FROM jsonb_path_query(input, '$.videos[*].analyzer') AS slot(body) "
    "UNION ALL SELECT slot.body #>> '{}' FROM jsonb_path_query(input, '$.images[*].analyzer') AS slot(body)) item "
    "LEFT JOIN pipeline_analyzer ON pipeline_analyzer.id::text = item.id WHERE item.id IS NOT NULL), '[]'::json)::text AS analyzer_names, ";

inline const char* QUERY_SEARCH = "search";
inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const char* QUERY_DATE = "date";
inline const char* QUERY_PAD = "pad";
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

}
