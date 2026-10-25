#include "services/pipeline_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/pipeline_const.hpp"
#include "searcher/searcher.hpp"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <stdexcept>

PipelineService::PipelineService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

Json::Value PipelineService::list(const std::string& search, int page, long pageSize) {
  const long offset = static_cast<long>(page - 1) * pageSize;
  const std::string columns =
      std::string("id::text AS id, name, type, status, NULL::text AS input, NULL::text AS config_path, ") +
      pipeline::SQL_REF_COLUMNS +
      "start_time, end_time, duration, updated_at ";
  const std::string match = Searcher::clause({"id", "name", "type", "status", "updated_at"}, 1);
  const std::string from = "FROM pipeline WHERE " + match + " ";
  const auto counted = Sql::sync({}, dbClient, "SELECT count(*) AS total " + from, search);
  const auto rows = Sql::sync({}, dbClient, 
      "SELECT " + columns + from + "ORDER BY name LIMIT $2 OFFSET $3", search, pageSize, offset);
  Json::Value items(Json::arrayValue);
  for (const auto& row : rows) {
    items.append(row2json(row));
  }
  Json::Value data;
  data[pipeline::KEY_ITEMS] = items;
  data[pipeline::KEY_TOTAL] = counted[0]["total"].as<long>();
  data[pipeline::KEY_PAGE] = page;
  data[pipeline::KEY_PAGE_SIZE] = pageSize;
  return data;
}

Json::Value PipelineService::map() {
  const auto rows = Sql::sync({}, dbClient, "SELECT id::text AS id, name FROM pipeline ORDER BY name");
  Json::Value data(Json::objectValue);
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value PipelineService::get(const std::string& pipelineId) {
  const auto rows = Sql::sync({}, dbClient, 
      std::string("SELECT id::text AS id, name, type, status, input::text AS input, config_path, ") +
      pipeline::SQL_REF_COLUMNS +
      "start_time, end_time, duration, updated_at "
      "FROM pipeline WHERE id = $1::uuid",
      pipelineId);
  if (rows.empty()) {
    throw std::runtime_error("pipeline not found");
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

YAML::Node PipelineService::json2yaml(const Json::Value& value) {
  YAML::Node node;
  if (value.isObject()) {
    node = YAML::Node(YAML::NodeType::Map);
    for (const auto& name : value.getMemberNames()) {
      node[name] = json2yaml(value[name]);
    }
  } else if (value.isArray()) {
    node = YAML::Node(YAML::NodeType::Sequence);
    for (const auto& item : value) {
      node.push_back(json2yaml(item));
    }
  } else if (value.isBool()) {
    node = value.asBool();
  } else if (value.isInt64()) {
    node = static_cast<long long>(value.asInt64());
  } else if (value.isUInt64()) {
    node = static_cast<unsigned long long>(value.asUInt64());
  } else if (value.isDouble()) {
    node = value.asDouble();
  } else if (value.isString()) {
    node = value.asString();
  } else if (value.isNull()) {
    node = YAML::Null;
  }
  return node;
}

std::string PipelineService::createStart(const Json::Value& input) {
  YAML::Emitter emitter;
  emitter << json2yaml(input);
  const bool emitted = emitter.good();
  if (!emitted) {
    throw std::runtime_error("pipeline yaml not built");
  }
  std::string yaml = emitter.c_str();
  return yaml;
}

Json::Value PipelineService::createEnd(const Json::Value& status,
                                        const Json::Value& configPath,
                                        const Json::Value& name,
                                        const Json::Value& type,
                                        const Json::Value& analyzerIds,
                                        const Json::Value& streamIds,
                                        const Json::Value& gieIds,
                                        const Json::Value& input) {
  Json::Value data = pipeline::ROW();
  data[pipeline::KEY_NAME] = name.isNull() ? data[pipeline::KEY_NAME] : name;
  data[pipeline::KEY_TYPE] = type.isNull() ? data[pipeline::KEY_TYPE] : type;
  data[pipeline::KEY_STATUS] = status.isNull() ? data[pipeline::KEY_STATUS] : status;
  data[pipeline::KEY_INPUT] = input.isNull() ? data[pipeline::KEY_INPUT] : input;
  data[pipeline::KEY_CONFIG_PATH] = configPath.isNull() ? data[pipeline::KEY_CONFIG_PATH] : configPath;
  data[pipeline::KEY_STREAM_IDS] = streamIds.isNull() ? Json::Value(Json::arrayValue) : streamIds;
  data[pipeline::KEY_GIE_IDS] = gieIds.isNull() ? data[pipeline::KEY_GIE_IDS] : gieIds;
  data[pipeline::KEY_ANALYZER_IDS] = analyzerIds.isNull() ? data[pipeline::KEY_ANALYZER_IDS] : analyzerIds;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const auto rows = Sql::sync({}, dbClient, 
      "INSERT INTO pipeline (id, name, type, status, input, config_path, stream_ids, gie_ids, analyzer_ids, updated_at) "
      "VALUES (gen_random_uuid(), $1, $2, $3, $4::jsonb, $5, "
      "ARRAY(SELECT jsonb_array_elements_text($6::jsonb)::uuid), "
      "ARRAY(SELECT jsonb_array_elements_text($7::jsonb)::uuid), "
      "ARRAY(SELECT jsonb_array_elements_text($8::jsonb)::uuid), now()) " +
      std::string("RETURNING id::text AS id, name, type, status, input::text AS input, config_path, ") +
      pipeline::SQL_REF_COLUMNS +
      "start_time, end_time, duration, updated_at",
      data[pipeline::KEY_NAME].asString(),
      data[pipeline::KEY_TYPE].asString(),
      data[pipeline::KEY_STATUS].asString(),
      Json::writeString(builder, data[pipeline::KEY_INPUT]),
      data[pipeline::KEY_CONFIG_PATH].asString(),
      Json::writeString(builder, data[pipeline::KEY_STREAM_IDS]),
      Json::writeString(builder, data[pipeline::KEY_GIE_IDS]),
      Json::writeString(builder, data[pipeline::KEY_ANALYZER_IDS]));
  if (rows.empty()) {
    throw std::runtime_error("pipeline not created");
  }
  Json::Value result = row2json(rows[0]);
  return result;
}

Json::Value PipelineService::update(const std::string& pipelineId,
                                        const Json::Value& status,
                                        const Json::Value& configPath,
                                        const Json::Value& name,
                                        const Json::Value& type,
                                        const Json::Value& analyzerIds,
                                        const Json::Value& gieIds,
                                        const Json::Value& input) {
  const Json::Value current = get(pipelineId);
  const bool hasPath = current[pipeline::KEY_CONFIG_PATH].isString();
  const std::string oldPath = hasPath ? current[pipeline::KEY_CONFIG_PATH].asString() : "";
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const auto rows = Sql::sync({}, dbClient, 
      "UPDATE pipeline SET name = $2, type = $3, status = $4, input = $5::jsonb, config_path = $6, "
      "gie_ids = ARRAY(SELECT jsonb_array_elements_text($7::jsonb)::uuid), "
      "analyzer_ids = ARRAY(SELECT jsonb_array_elements_text($8::jsonb)::uuid), "
      "start_time = NULL, end_time = NULL, duration = NULL, updated_at = now() "
      "WHERE id = $1::uuid AND status <> $9 " +
      std::string("RETURNING id::text AS id, name, type, status, input::text AS input, config_path, ") +
      pipeline::SQL_REF_COLUMNS +
      "start_time, end_time, duration, updated_at",
      pipelineId,
      name.asString(),
      type.asString(),
      status.asString(),
      Json::writeString(builder, input),
      configPath.asString(),
      Json::writeString(builder, gieIds),
      Json::writeString(builder, analyzerIds),
      std::string(pipeline::STATUS_STARTING));
  if (rows.empty()) {
    throw std::runtime_error(pipeline::ERR_BUSY);
  }
  if (hasPath && oldPath != configPath.asString()) {
    std::filesystem::remove(oldPath);
  }
  Json::Value result = row2json(rows[0]);
  return result;
}

Json::Value PipelineService::setStatus(const std::string& pipelineId, const std::string& status) {
  const auto rows = Sql::sync({}, dbClient, 
      "UPDATE pipeline SET status = $2 "
      "WHERE id = $1::uuid " +
      std::string("RETURNING id::text AS id, name, type, status, input::text AS input, config_path, ") +
      pipeline::SQL_REF_COLUMNS +
      "start_time, end_time, duration, updated_at",
      pipelineId,
      status);
  if (rows.empty()) {
    throw std::runtime_error("pipeline not updated");
  }
  Json::Value result = row2json(rows[0]);
  return result;
}

Json::Value PipelineService::probe(const std::string& pipelineId,
                                   const std::string& status,
                                   const Json::Value& startTime,
                                   const Json::Value& endTime,
                                   const Json::Value& duration) {
  const std::string startText = startTime.isString() ? startTime.asString() : "";
  const std::string endText = endTime.isString() ? endTime.asString() : "";
  const std::string durationText = duration.isNumeric() ? std::to_string(duration.asInt64()) : "";
  const auto rows = Sql::sync({}, dbClient, 
      "UPDATE pipeline SET status = $2, "
      "start_time = CASE WHEN $3 = '' THEN NULL ELSE $3::timestamptz END, "
      "end_time = CASE WHEN $4 = '' THEN NULL ELSE $4::timestamptz END, "
      "duration = CASE WHEN $5 = '' THEN NULL ELSE $5::bigint END "
      "WHERE id = $1::uuid " +
      std::string("RETURNING id::text AS id, name, type, status, input::text AS input, config_path, ") +
      pipeline::SQL_REF_COLUMNS +
      "start_time, end_time, duration, updated_at",
      pipelineId,
      status,
      startText,
      endText,
      durationText);
  if (rows.empty()) {
    throw std::runtime_error("pipeline not updated");
  }
  Json::Value result = row2json(rows[0]);
  return result;
}

Json::Value PipelineService::remove(const std::string& pipelineId) {
  const auto rows = Sql::sync({}, dbClient, 
      "DELETE FROM pipeline WHERE id = $1::uuid " +
      std::string("RETURNING id::text AS id, name, type, status, input::text AS input, config_path, ") +
      pipeline::SQL_REF_COLUMNS +
      "start_time, end_time, duration, updated_at",
      pipelineId);
  if (rows.empty()) {
    throw std::runtime_error("pipeline not removed");
  }
  const bool hasPath = !rows[0]["config_path"].isNull();
  if (hasPath) {
    std::filesystem::remove(rows[0]["config_path"].as<std::string>());
  }
  Json::Value result = row2json(rows[0]);
  return result;
}

Json::Value PipelineService::json2array(const std::string& text) {
  Json::Value parsed;
  const bool array = Json::Reader().parse(text, parsed) && parsed.isArray();
  Json::Value result = array ? parsed : Json::Value(Json::arrayValue);
  return result;
}

Json::Value PipelineService::row2json(const drogon::orm::Row& row) {
  Json::Value node = pipeline::ROW();
  node[pipeline::KEY_ID] = row["id"].as<std::string>();
  node[pipeline::KEY_NAME] = row["name"].as<std::string>();
  node[pipeline::KEY_TYPE] = row["type"].as<std::string>();
  node[pipeline::KEY_STATUS] = row["status"].as<std::string>();
  Json::Value input;
  const bool parsed = !row["input"].isNull() && Json::Reader().parse(row["input"].as<std::string>(), input);
  node[pipeline::KEY_INPUT] = parsed ? input : Json::Value();
  node[pipeline::KEY_CONFIG_PATH] = row["config_path"].isNull() ? Json::Value() : row["config_path"].as<std::string>();
  node[pipeline::KEY_STREAM_IDS] = json2array(row["stream_ids"].as<std::string>());
  node[pipeline::KEY_STREAM_NAMES] = json2array(row["stream_names"].as<std::string>());
  node[pipeline::KEY_GIE_IDS] = json2array(row["gie_ids"].as<std::string>());
  node[pipeline::KEY_GIE_NAMES] = json2array(row["gie_names"].as<std::string>());
  node[pipeline::KEY_ANALYZER_IDS] = json2array(row["analyzer_ids"].as<std::string>());
  node[pipeline::KEY_ANALYZER_NAMES] = json2array(row["analyzer_names"].as<std::string>());
  node[pipeline::KEY_START_TIME] = row["start_time"].isNull() ? Json::Value() : row["start_time"].as<std::string>();
  node[pipeline::KEY_END_TIME] = row["end_time"].isNull() ? Json::Value() : row["end_time"].as<std::string>();
  node[pipeline::KEY_DURATION] = row["duration"].isNull() ? Json::Value() : Json::Value(static_cast<Json::Int64>(row["duration"].as<long>()));
  node[pipeline::KEY_UPDATED_AT] = row["updated_at"].isNull() ? Json::Value() : row["updated_at"].as<std::string>();
  return node;
}
