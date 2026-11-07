#include "services/pipeline_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/pipeline_const.hpp"
#include "searcher/searcher.hpp"

#include <yaml-cpp/yaml.h>

#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include <zip.h>

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

YAML::Node PipelineService::readParams(const std::string& configPath) {
  int error = 0;
  zip_t* archive = zip_open(configPath.c_str(), ZIP_RDONLY, &error);
  std::string paramsBytes;
  const zip_int64_t count = archive ? zip_get_num_entries(archive, 0) : 0;
  for (zip_int64_t index = 0; index < count; ++index) {
    const char* entryName = zip_get_name(archive, index, 0);
    const std::string name = entryName ? entryName : "";
    const bool directoryEntry = !name.empty() && name.back() == '/';
    zip_stat_t stat;
    const int stated = directoryEntry ? -1 : zip_stat_index(archive, index, 0, &stat);
    const bool sized = stated == 0 && (stat.valid & ZIP_STAT_SIZE) != 0;
    zip_file_t* entry = sized ? zip_fopen_index(archive, index, 0) : nullptr;
    std::string bytes(sized ? static_cast<std::size_t>(stat.size) : 0, '\0');
    const zip_int64_t read = entry ? zip_fread(entry, bytes.data(), bytes.size()) : -1;
    if (entry) {
      zip_fclose(entry);
    }
    const bool paramsFile = read == static_cast<zip_int64_t>(bytes.size()) &&
                            std::filesystem::path(name).filename() == pipeline::PARAMS_YML;
    paramsBytes = paramsFile ? bytes : paramsBytes;
  }
  if (archive) {
    zip_close(archive);
  }
  if (paramsBytes.empty()) {
    throw std::runtime_error("params.yml missing");
  }
  return YAML::Load(paramsBytes);
}

void PipelineService::collectModelZips(const YAML::Node& node, std::vector<std::string>& paths) {
  const bool map = node && node.IsMap();
  const bool sequence = node && node.IsSequence();
  if (map) {
    for (const auto& item : node) {
      const bool modelZip = item.first.as<std::string>() == pipeline::KEY_MODEL_ZIP && item.second.IsScalar();
      if (modelZip) {
        paths.push_back(item.second.as<std::string>());
      }
      collectModelZips(item.second, paths);
    }
  }
  if (sequence) {
    for (const auto& item : node) {
      collectModelZips(item, paths);
    }
  }
}

void PipelineService::collectMedia(const YAML::Node& params, std::vector<std::string>& paths) {
  const char* keys[] = {pipeline::KEY_VIDEOS, pipeline::KEY_IMAGES};
  for (const char* key : keys) {
    const YAML::Node node = params[key];
    const bool sequence = node && node.IsSequence();
    if (sequence) {
      for (const auto& item : node) {
        const bool scalar = item.IsScalar();
        if (scalar) {
          paths.push_back(item.as<std::string>());
        }
      }
    }
  }
}

std::vector<std::string> PipelineService::getModelPaths(const YAML::Node& params) {
  std::vector<std::string> paths;
  collectModelZips(params, paths);
  return paths;
}

std::string PipelineService::packMedia(const YAML::Node& params) {
  std::vector<std::string> paths;
  collectMedia(params, paths);
  zip_error_t error;
  zip_error_init(&error);
  zip_source_t* buffer = paths.empty() ? nullptr : zip_source_buffer_create(nullptr, 0, 1, &error);
  if (buffer != nullptr) {
    zip_source_keep(buffer);
  }
  zip_t* archive = buffer == nullptr ? nullptr : zip_open_from_source(buffer, ZIP_CREATE, &error);
  bool packed = paths.empty() || archive != nullptr;
  for (const std::string& path : paths) {
    const std::filesystem::path file(path);
    const bool exists = std::filesystem::is_regular_file(file);
    std::string entry = file.generic_string();
    if (!entry.empty() && entry.front() == '/') {
      entry.erase(entry.begin());
    }
    bool safe = exists && !entry.empty();
    for (const std::filesystem::path& part : std::filesystem::path(entry)) {
      safe = safe && part != ".." && part != ".";
    }
    zip_source_t* source = packed && safe && archive != nullptr ? zip_source_file(archive, path.c_str(), 0, -1) : nullptr;
    const zip_int64_t added = source == nullptr ? -1 : zip_file_add(archive, entry.c_str(), source, ZIP_FL_ENC_UTF_8 | ZIP_FL_OVERWRITE);
    if (added < 0 && source != nullptr) {
      zip_source_free(source);
    }
    packed = packed && added >= 0;
  }
  if (!packed && archive != nullptr) {
    zip_discard(archive);
    archive = nullptr;
  }
  if (packed && archive != nullptr && zip_close(archive) < 0) {
    zip_discard(archive);
    packed = false;
  }
  const bool readable = packed && buffer != nullptr && zip_source_open(buffer) == 0 && zip_source_seek(buffer, 0, SEEK_END) == 0;
  const zip_int64_t size = readable ? zip_source_tell(buffer) : -1;
  if (readable) {
    zip_source_seek(buffer, 0, SEEK_SET);
  }
  std::string bytes(size > 0 ? static_cast<std::size_t>(size) : 0, '\0');
  const zip_int64_t read = readable && size > 0 ? zip_source_read(buffer, bytes.data(), bytes.size()) : 0;
  if (readable) {
    zip_source_close(buffer);
  }
  if (buffer != nullptr) {
    zip_source_free(buffer);
  }
  zip_error_fini(&error);
  if (!packed || (size > 0 && read != size)) {
    throw std::runtime_error("media zip failed");
  }
  return bytes;
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
