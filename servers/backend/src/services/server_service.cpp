#include "services/server_service.hpp"

#include "constants/server_const.hpp"

ServerService::ServerService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

Json::Value ServerService::get() {
  const auto rows = dbClient->execSqlSync(
      "SELECT id::text AS id, probed_at::text AS probed_at, deepstream::text AS deepstream, generator, "
      "export_onnx, export_trt, mediamtx, kafka, postgresql, frontend, backend "
      "FROM server WHERE id = $1::uuid",
      server::ROW_ID);
  return row2json(rows[0]);
}

drogon::Task<bool> ServerService::healthy(const std::string& baseUrl, const std::string& path) {
  bool ok = false;
  try {
    auto request = drogon::HttpRequest::newHttpRequest();
    request->setMethod(drogon::Get);
    request->setPath(path);
    auto client = drogon::HttpClient::newHttpClient(baseUrl);
    const auto response = co_await client->sendRequestCoro(request, server::HEALTH_TIMEOUT_SECONDS);
    ok = response->statusCode() >= drogon::k200OK && response->statusCode() < drogon::k300MultipleChoices;
  } catch (const std::exception&) {
    ok = false;
  }
  co_return ok;
}

Json::Value ServerService::row2json(const drogon::orm::Row& row) {
  Json::Value data = server::ROW();
  data[server::KEY_ID] = row["id"].as<std::string>();
  data[server::KEY_PROBED_AT] = row["probed_at"].isNull() ? Json::Value() : row["probed_at"].as<std::string>();
  Json::Value deepstream;
  Json::Reader reader;
  const bool parsed = !row["deepstream"].isNull() && reader.parse(row["deepstream"].as<std::string>(), deepstream);
  data[server::KEY_DEEPSTREAM] = parsed ? deepstream : Json::Value(Json::arrayValue);
  data[server::KEY_GENERATOR] = row["generator"].as<bool>();
  data[server::KEY_EXPORT_ONNX] = row["export_onnx"].as<bool>();
  data[server::KEY_EXPORT_TRT] = row["export_trt"].as<bool>();
  data[server::KEY_MEDIAMTX] = row["mediamtx"].as<bool>();
  data[server::KEY_KAFKA] = row["kafka"].as<bool>();
  data[server::KEY_POSTGRESQL] = row["postgresql"].as<bool>();
  data[server::KEY_FRONTEND] = row["frontend"].as<bool>();
  data[server::KEY_BACKEND] = row["backend"].as<bool>();
  return data;
}

drogon::Task<Json::Value> ServerService::probe() {
  const auto pipelines = dbClient->execSqlSync("SELECT id::text AS id, name FROM pipeline ORDER BY name");
  Json::Value deepstream(Json::arrayValue);
  for (const auto& pipeline : pipelines) {
    const std::string name = pipeline["name"].as<std::string>();
    const std::string baseUrl = std::string("http://") + server::CONTAINER_PREFIX + name + ":" +
                                 std::to_string(server::DEEPSTREAM_PORT);
    Json::Value item;
    item[server::KEY_ID] = pipeline["id"].as<std::string>();
    item[server::KEY_NAME] = name;
    item[server::KEY_HEALTHY] = co_await healthy(baseUrl, server::DEEPSTREAM_HEALTH_PATH);
    deepstream.append(item);
  }
  const bool generator = co_await healthy(server::GENERATOR_BASE_URL, server::GENERATOR_HEALTH_PATH);
  const bool exportOnnx = co_await healthy(server::EXPORT_ONNX_BASE_URL, server::EXPORT_ONNX_HEALTH_PATH);
  const bool exportTrt = co_await healthy(server::EXPORT_TRT_BASE_URL, server::EXPORT_TRT_HEALTH_PATH);
  const bool mediamtx = co_await healthy(server::MEDIAMTX_BASE_URL, server::MEDIAMTX_HEALTH_PATH);
  const bool kafka = co_await healthy(server::KAFKA_BASE_URL, server::KAFKA_HEALTH_PATH);
  const bool frontend = co_await healthy(server::FRONTEND_BASE_URL, server::FRONTEND_HEALTH_PATH);
  const bool backend = co_await healthy(server::BACKEND_BASE_URL, server::BACKEND_HEALTH_PATH);
  bool postgresql = false;
  try {
    dbClient->execSqlSync(server::POSTGRESQL_HEALTH_SQL);
    postgresql = true;
  } catch (const std::exception&) {
    postgresql = false;
  }
  Json::StreamWriterBuilder builder;
  const auto saved = dbClient->execSqlSync(
      "UPDATE server SET probed_at = now(), deepstream = $2::jsonb, generator = $3, export_onnx = $4, "
      "export_trt = $5, mediamtx = $6, kafka = $7, postgresql = $8, frontend = $9, backend = $10 "
      "WHERE id = $1::uuid "
      "RETURNING id::text AS id, probed_at::text AS probed_at, deepstream::text AS deepstream, generator, "
      "export_onnx, export_trt, mediamtx, kafka, postgresql, frontend, backend",
      server::ROW_ID,
      Json::writeString(builder, deepstream),
      generator,
      exportOnnx,
      exportTrt,
      mediamtx,
      kafka,
      postgresql,
      frontend,
      backend);
  co_return row2json(saved[0]);
}
