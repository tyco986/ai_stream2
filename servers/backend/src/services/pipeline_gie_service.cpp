#include "services/pipeline_gie_service.hpp"

#include "constants/pipeline_gie_const.hpp"

#include <stdexcept>

PipelineGieService::PipelineGieService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

Json::Value PipelineGieService::getModel() {
  const auto rows = dbClient->execSqlSync("SELECT id::text AS id, name, status FROM model ORDER BY name");
  Json::Value data(Json::objectValue);
  for (const auto& row : rows) {
    Json::Value item;
    item[pipeline_gie::KEY_ID] = row["id"].as<std::string>();
    item[pipeline_gie::KEY_STATUS] = row["status"].as<std::string>();
    data[row["name"].as<std::string>()] = item;
  }
  return data;
}

Json::Value PipelineGieService::map() {
  const auto rows = dbClient->execSqlSync("SELECT id::text AS id, name FROM pipeline_gie ORDER BY name");
  Json::Value data(Json::objectValue);
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value PipelineGieService::get(const std::string& gieId) {
  const auto rows = dbClient->execSqlSync(
      "SELECT g.id::text AS id, g.name, g.model_id::text AS model_id, g.model_name, "
      "g.class_attrs::text AS class_attrs, g.interval, g.updated_at, m.engine_path "
      "FROM pipeline_gie g JOIN model m ON m.id = g.model_id WHERE g.id = $1::uuid",
      gieId);
  if (rows.empty()) {
    throw std::runtime_error(pipeline_gie::ERR_NOT_FOUND);
  }
  Json::Value data = row2json(rows[0], rows[0]["engine_path"].as<std::string>());
  return data;
}

Json::Value PipelineGieService::create(const Json::Value& modelId,
                                       const Json::Value& name,
                                       const Json::Value& classAttrs,
                                       const Json::Value& interval) {
  const auto models = dbClient->execSqlSync(
      "SELECT name, engine_path FROM model WHERE id = $1::uuid", modelId.asString());
  if (models.empty()) {
    throw std::runtime_error("model not found");
  }
  Json::Value data = pipeline_gie::ROW();
  data[pipeline_gie::KEY_NAME] = name.isNull() ? data[pipeline_gie::KEY_NAME] : name;
  data[pipeline_gie::KEY_MODEL_ID] = modelId.isNull() ? data[pipeline_gie::KEY_MODEL_ID] : modelId;
  data[pipeline_gie::KEY_MODEL_NAME] = models[0]["name"].as<std::string>();
  data[pipeline_gie::KEY_CLASS_ATTRS] = classAttrs.isNull() ? data[pipeline_gie::KEY_CLASS_ATTRS] : classAttrs;
  data[pipeline_gie::KEY_INTERVAL] = interval.isNull() ? data[pipeline_gie::KEY_INTERVAL] : interval;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const auto rows = dbClient->execSqlSync(
      "INSERT INTO pipeline_gie (id, name, model_id, model_name, class_attrs, interval, updated_at) "
      "VALUES (gen_random_uuid(), $1, $2::uuid, $3, $4::jsonb, $5, now()) "
      "RETURNING id::text AS id, name, model_id::text AS model_id, model_name, "
      "class_attrs::text AS class_attrs, interval, updated_at",
      data[pipeline_gie::KEY_NAME].asString(),
      data[pipeline_gie::KEY_MODEL_ID].asString(),
      data[pipeline_gie::KEY_MODEL_NAME].asString(),
      Json::writeString(builder, data[pipeline_gie::KEY_CLASS_ATTRS]),
      data[pipeline_gie::KEY_INTERVAL].asInt());
  data = row2json(rows[0], models[0]["engine_path"].as<std::string>());
  return data;
}

Json::Value PipelineGieService::update(const std::string& gieId,
                                       const Json::Value& modelId,
                                       const Json::Value& name,
                                       const Json::Value& classAttrs,
                                       const Json::Value& interval) {
  const auto models = dbClient->execSqlSync(
      "SELECT name, engine_path FROM model WHERE id = $1::uuid", modelId.asString());
  if (models.empty()) {
    throw std::runtime_error("model not found");
  }
  Json::Value data = pipeline_gie::ROW();
  data[pipeline_gie::KEY_NAME] = name.isNull() ? data[pipeline_gie::KEY_NAME] : name;
  data[pipeline_gie::KEY_MODEL_ID] = modelId.isNull() ? data[pipeline_gie::KEY_MODEL_ID] : modelId;
  data[pipeline_gie::KEY_MODEL_NAME] = models[0]["name"].as<std::string>();
  data[pipeline_gie::KEY_CLASS_ATTRS] = classAttrs.isNull() ? data[pipeline_gie::KEY_CLASS_ATTRS] : classAttrs;
  data[pipeline_gie::KEY_INTERVAL] = interval.isNull() ? data[pipeline_gie::KEY_INTERVAL] : interval;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const auto rows = dbClient->execSqlSync(
      "UPDATE pipeline_gie SET name = $2, model_id = $3::uuid, model_name = $4, class_attrs = $5::jsonb, "
      "interval = $6, updated_at = now() WHERE id = $1::uuid "
      "RETURNING id::text AS id, name, model_id::text AS model_id, model_name, "
      "class_attrs::text AS class_attrs, interval, updated_at",
      gieId,
      data[pipeline_gie::KEY_NAME].asString(),
      data[pipeline_gie::KEY_MODEL_ID].asString(),
      data[pipeline_gie::KEY_MODEL_NAME].asString(),
      Json::writeString(builder, data[pipeline_gie::KEY_CLASS_ATTRS]),
      data[pipeline_gie::KEY_INTERVAL].asInt());
  if (rows.empty()) {
    throw std::runtime_error(pipeline_gie::ERR_NOT_FOUND);
  }
  data = row2json(rows[0], models[0]["engine_path"].as<std::string>());
  return data;
}

Json::Value PipelineGieService::remove(const std::string& gieId) {
  const auto used = dbClient->execSqlSync(
      "SELECT id FROM pipeline WHERE $1::uuid = ANY(gie_ids) LIMIT 1", gieId);
  if (!used.empty()) {
    throw std::runtime_error(pipeline_gie::ERR_OCCUPIED);
  }
  const auto rows = dbClient->execSqlSync(
      "DELETE FROM pipeline_gie AS g USING model AS m "
      "WHERE g.id = $1::uuid AND m.id = g.model_id "
      "RETURNING g.id::text AS id, g.name, g.model_id::text AS model_id, g.model_name, "
      "g.class_attrs::text AS class_attrs, g.interval, g.updated_at, m.engine_path",
      gieId);
  if (rows.empty()) {
    throw std::runtime_error(pipeline_gie::ERR_NOT_FOUND);
  }
  const std::string enginePath = rows[0]["engine_path"].isNull() ? "" : rows[0]["engine_path"].as<std::string>();
  Json::Value data = row2json(rows[0], enginePath);
  return data;
}

Json::Value PipelineGieService::row2json(const drogon::orm::Row& row, const std::string& enginePath) {
  Json::Value classAttrs;
  Json::Reader().parse(row["class_attrs"].as<std::string>(), classAttrs);
  Json::Value data = pipeline_gie::ROW();
  data[pipeline_gie::KEY_ID] = row["id"].as<std::string>();
  data[pipeline_gie::KEY_NAME] = row["name"].as<std::string>();
  data[pipeline_gie::KEY_MODEL_ID] = row["model_id"].as<std::string>();
  data[pipeline_gie::KEY_MODEL_NAME] = row["model_name"].as<std::string>();
  data[pipeline_gie::KEY_CLASS_ATTRS] = classAttrs;
  data[pipeline_gie::KEY_INTERVAL] = row["interval"].as<int>();
  data[pipeline_gie::KEY_ENGINE_PATH] = enginePath;
  data[pipeline_gie::KEY_UPDATED_AT] = row["updated_at"].as<std::string>();
  return data;
}
