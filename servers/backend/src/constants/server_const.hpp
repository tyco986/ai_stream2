#pragma once

#include <json/json.h>

namespace server {

inline const char* KEY_ID = "id";
inline const char* KEY_PROBED_AT = "probed_at";
inline const char* KEY_DEEPSTREAM = "deepstream";
inline const char* KEY_GENERATOR = "generator";
inline const char* KEY_EXPORT_ONNX = "export_onnx";
inline const char* KEY_EXPORT_TRT = "export_trt";
inline const char* KEY_MEDIAMTX = "mediamtx";
inline const char* KEY_KAFKA = "kafka";
inline const char* KEY_POSTGRESQL = "postgresql";
inline const char* KEY_FRONTEND = "frontend";
inline const char* KEY_BACKEND = "backend";
inline const char* KEY_NAME = "name";
inline const char* KEY_HEALTHY = "healthy";

inline const char* PATH_ITEM = "/ai_stream2/backend/server";
inline const char* PATH_PROBE = "/ai_stream2/backend/server/probe";
inline const char* MSG_GET_OK = "server_get_ok";
inline const char* MSG_GET_ERROR = "server_get_error";
inline const char* MSG_PROBE_OK = "server_probe_ok";
inline const char* MSG_PROBE_ERROR = "server_probe_error";

inline const char* ROW_ID = "00000000-0000-4000-8000-000000000005";
inline const char* CONTAINER_PREFIX = "puskas_";
inline const int DEEPSTREAM_PORT = 9000;
inline const char* DEEPSTREAM_HEALTH_PATH = "/health";
inline const char* GENERATOR_BASE_URL = "http://puskas_generator:8091";
inline const char* GENERATOR_HEALTH_PATH = "/health";
inline const char* EXPORT_ONNX_BASE_URL = "http://puskas_export_onnx:8090";
inline const char* EXPORT_ONNX_HEALTH_PATH = "/health";
inline const char* EXPORT_TRT_BASE_URL = "http://puskas_export_trt:9000";
inline const char* EXPORT_TRT_HEALTH_PATH = "/ai_stream2/export_trt/health";
inline const char* MEDIAMTX_BASE_URL = "http://ai_stream2_mediamtx:9997";
inline const char* MEDIAMTX_HEALTH_PATH = "/v3/info";
inline const char* KAFKA_BASE_URL = "http://ai_stream2_kafka:9644";
inline const char* KAFKA_HEALTH_PATH = "/v1/status/ready";
inline const char* FRONTEND_BASE_URL = "http://puskas_nodejs:5173";
inline const char* FRONTEND_HEALTH_PATH = "/health";
inline const char* BACKEND_BASE_URL = "http://127.0.0.1:8000";
inline const char* BACKEND_HEALTH_PATH = "/health";
inline const char* POSTGRESQL_HEALTH_SQL = "SELECT 1";
inline const double HEALTH_TIMEOUT_SECONDS = 3.0;

inline Json::Value ROW() {
  Json::Value node;
  node[KEY_ID] = Json::Value();
  node[KEY_PROBED_AT] = Json::Value();
  node[KEY_DEEPSTREAM] = Json::Value(Json::arrayValue);
  node[KEY_GENERATOR] = false;
  node[KEY_EXPORT_ONNX] = false;
  node[KEY_EXPORT_TRT] = false;
  node[KEY_MEDIAMTX] = false;
  node[KEY_KAFKA] = false;
  node[KEY_POSTGRESQL] = false;
  node[KEY_FRONTEND] = false;
  node[KEY_BACKEND] = false;
  return node;
}

}
