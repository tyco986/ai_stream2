#pragma once

#include <json/json.h>

namespace pipeline {

inline const char* PATH_LIST = "/ai_stream2/backend/pipelines";
inline const char* PATH_MAP = "/ai_stream2/backend/pipelines/map";
inline const char* PATH_ITEM = "/ai_stream2/backend/pipelines/{pipeline_id}";
inline const char* PATH_START = "/ai_stream2/backend/pipelines/{pipeline_id}/start";
inline const char* PATH_PROBE = "/ai_stream2/backend/pipelines/{pipeline_id}/probe";
inline const char* PATH_STOP = "/ai_stream2/backend/pipelines/{pipeline_id}/stop";
inline const char* PATH_GENERATE = "/ai_stream2/generator/generate";

inline const char* KEY_ID = "id";
inline const char* KEY_NAME = "name";
inline const char* KEY_TYPE = "type";
inline const char* KEY_STATUS = "status";
inline const char* KEY_INPUT = "input";
inline const char* KEY_CONFIG_PATH = "config_path";
inline const char* KEY_GIE_ID = "gie_id";
inline const char* KEY_ANALYZER_ID = "analyzer_id";
inline const char* KEY_STREAM_IDS = "stream_ids";
inline const char* KEY_STREAM_NAMES = "stream_names";
inline const char* KEY_GIE_IDS = "gie_ids";
inline const char* KEY_GIE_NAMES = "gie_names";
inline const char* KEY_ANALYZER_IDS = "analyzer_ids";
inline const char* KEY_ANALYZER_NAMES = "analyzer_names";
inline const char* KEY_PIPELINE_NAME = "pipeline_name";
inline const char* KEY_GENERATOR = "generator";
inline const char* KEY_START_TIME = "start_time";
inline const char* KEY_END_TIME = "end_time";
inline const char* KEY_DURATION = "duration";
inline const char* KEY_UPDATED_AT = "updated_at";
inline const char* KEY_ITEMS = "items";
inline const char* KEY_TOTAL = "total";
inline const char* KEY_PAGE = "page";
inline const char* KEY_PAGE_SIZE = "page_size";

inline const char* FIELD_INPUT = "input";
inline const char* CONFIG_FILENAME = "input.yaml";
inline const char* MULTIPART_BOUNDARY = "ai_stream2_pipeline_generate";
inline const char* MULTIPART_CONTENT_TYPE = "multipart/form-data; boundary=ai_stream2_pipeline_generate";

inline const char* CONFIG_DIR = "/root/configs";
inline const char* SUFFIX_ZIP = ".zip";
inline const char* STATUS_STOPPED = "stopped";
inline const char* STATUS_STARTING = "starting";
inline const char* STATUS_RUNNING = "running";
inline const char* ERR_BUSY = "pipeline is running";

inline const char* DOCKER_BASE_URL = "http://puskas_docker_socket_proxy:2375";
inline const char* DOCKER_CREATE_PATH = "/containers/create";
inline const char* DOCKER_STOP_PATH = "/stop";
inline const char* DOCKER_RESTART_PATH = "/restart";
inline const char* DOCKER_JSON_PATH = "/json";
inline const char* DOCKER_NETWORK = "puskas_default";
inline const char* DEEPSTREAM_IMAGE = "puskas_deepstream";
inline const char* CONTAINER_PREFIX = "puskas_";
inline const int DEEPSTREAM_PORT = 9000;
inline const char* DEEPSTREAM_HEALTH_PATH = "/health";
inline const char* DEEPSTREAM_START_PATH = "/start";
inline const char* DEEPSTREAM_STATUS_PATH = "/status";
inline const char* ZIP_FILENAME = "config.zip";
inline const int HEALTH_ATTEMPTS = 15;
inline const int STATUS_ATTEMPTS = 15;
inline const double HEALTH_INTERVAL_SECONDS = 1.0;
inline const double GENERATE_TIMEOUT_SECONDS = 3.0;
inline const double DOCKER_TIMEOUT_SECONDS = 5.0;
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
  node[KEY_GIE_IDS] = Json::Value(Json::arrayValue);
  node[KEY_ANALYZER_IDS] = Json::Value(Json::arrayValue);
  node[KEY_START_TIME] = Json::Value();
  node[KEY_END_TIME] = Json::Value();
  node[KEY_DURATION] = Json::Value();
  node[KEY_UPDATED_AT] = Json::Value();
  return node;
}

inline const char* MSG_LIST_OK = "Loaded pipelines";
inline const char* MSG_LIST_ERROR = "Failed to load pipelines";
inline const char* MSG_MAP_OK = "Loaded pipeline map";
inline const char* MSG_MAP_ERROR = "Failed to load pipeline map";
inline const char* MSG_GET_OK = "Loaded pipeline";
inline const char* MSG_GET_ERROR = "Failed to load pipeline";
inline const char* MSG_CREATE_OK = "Created pipeline";
inline const char* MSG_CREATE_ERROR = "Failed to create pipeline";
inline const char* MSG_UPDATE_OK = "Updated pipeline";
inline const char* MSG_UPDATE_ERROR = "Failed to update pipeline";
inline const char* MSG_REMOVE_OK = "Removed pipeline";
inline const char* MSG_REMOVE_ERROR = "Failed to remove pipeline";
inline const char* MSG_START_OK = "Started pipeline";
inline const char* MSG_START_ERROR = "Failed to start pipeline";
inline const char* MSG_STOP_OK = "Stopped pipeline";
inline const char* MSG_STOP_ERROR = "Failed to stop pipeline";
inline const char* MSG_PROBE_OK = "Probed pipeline";
inline const char* MSG_PROBE_ERROR = "Failed to probe pipeline";

inline const char* SQL_REF_COLUMNS =
    "array_to_json(stream_ids)::text AS stream_ids, "
    "COALESCE((SELECT json_agg(stream.name ORDER BY item.ord) FROM unnest(pipeline.stream_ids) WITH ORDINALITY AS item(id, ord) "
    "LEFT JOIN stream ON stream.id = item.id), '[]'::json)::text AS stream_names, "
    "array_to_json(gie_ids)::text AS gie_ids, "
    "COALESCE((SELECT json_agg(pipeline_gie.name ORDER BY item.ord) FROM unnest(pipeline.gie_ids) WITH ORDINALITY AS item(id, ord) "
    "LEFT JOIN pipeline_gie ON pipeline_gie.id = item.id), '[]'::json)::text AS gie_names, "
    "array_to_json(analyzer_ids)::text AS analyzer_ids, "
    "COALESCE((SELECT json_agg(pipeline_analyzer.name ORDER BY item.ord) FROM unnest(pipeline.analyzer_ids) WITH ORDINALITY AS item(id, ord) "
    "LEFT JOIN pipeline_analyzer ON pipeline_analyzer.id = item.id), '[]'::json)::text AS analyzer_names, ";

inline const char* QUERY_SEARCH = "search";
inline const char* QUERY_PAGE = "page";
inline const char* QUERY_PAGE_SIZE = "page_size";
inline const int DEFAULT_PAGE = 1;
inline const int DEFAULT_PAGE_SIZE = 20;

}
