#include "services/pipeline_service.hpp"

#include "error_tracer/sql.hpp"

#include "constants/pipeline_const.hpp"
#include "constants/pipeline_gie_const.hpp"
#include "searcher/searcher.hpp"

#include <yaml-cpp/yaml.h>

#include <cctype>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
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

bool PipelineService::queryDate(const std::string& date) {
  bool ok = date.size() == pipeline::DATE_LENGTH;
  for (std::size_t index = 0; ok && index < date.size(); ++index) {
    const bool separator = index == 4 || index == 7;
    const unsigned char character = static_cast<unsigned char>(date[index]);
    ok = separator ? date[index] == '-' : std::isdigit(character) != 0;
  }
  return ok;
}

Json::Value PipelineService::readJsonl(std::istream& input) {
  Json::Value lines(Json::arrayValue);
  for (std::string line; std::getline(input, line); ) {
    Json::Value item;
    const bool parsed = !line.empty() && Json::Reader().parse(line, item);
    const bool latencyField = parsed && item.isObject() && item[pipeline::KEY_LATENCY].isNumeric();
    if (latencyField) {
      std::ostringstream latency;
      latency << std::fixed << std::setprecision(pipeline::LATENCY_DIGITS) << item[pipeline::KEY_LATENCY].asDouble();
      item[pipeline::KEY_LATENCY] = latency.str();
    }
    if (parsed && item.isObject()) {
      lines.append(item);
    }
  }
  return lines;
}

bool PipelineService::queryPad(const std::string& pad) {
  bool ok = !pad.empty() && pad.size() <= static_cast<std::size_t>(pipeline::PAD_WIDTH);
  for (std::size_t index = 0; ok && index < pad.size(); ++index) {
    ok = std::isdigit(static_cast<unsigned char>(pad[index])) != 0;
  }
  const int value = ok ? std::stoi(pad) : 0;
  ok = ok && value < 1000;
  return ok;
}

Json::Value PipelineService::getPadLogs(const std::string& name, const std::string& pad, const std::string& date) {
  std::ostringstream folder;
  folder << std::setw(pipeline::PAD_WIDTH) << std::setfill('0') << std::stoi(pad);
  const std::filesystem::path directory =
      std::filesystem::path(pipeline::DEEPSTREAM_LOG_DIR) / name / pipeline::LOG_PAD_DIR / folder.str();
  const std::string suffix = std::string("_") + date + pipeline::LOG_API_SUFFIX;
  Json::Value lines(Json::arrayValue);
  const bool scan = std::filesystem::is_directory(directory);
  if (scan) {
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory)) {
      const std::string filename = entry.path().filename().string();
      const bool shaped = entry.is_regular_file() && filename.size() > suffix.size() &&
                          filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0;
      if (shaped) {
        std::ifstream file(entry.path());
        const Json::Value rows = readJsonl(file);
        for (const Json::Value& row : rows) {
          lines.append(row);
        }
      }
    }
  }
  Json::Value body(Json::objectValue);
  body[pipeline::KEY_CONTENT] = lines;
  return body;
}

Json::Value PipelineService::getApiLogs(const std::string& name, const std::string& date) {
  const std::filesystem::path directory =
      std::filesystem::path(pipeline::DEEPSTREAM_LOG_DIR) / name / pipeline::LOG_API_DIR;
  Json::Value dates(Json::objectValue);
  if (!date.empty()) {
    const std::filesystem::path direct =
        directory / (std::string(pipeline::LOG_API_PAGE) + "_" + date + pipeline::LOG_API_SUFFIX);
    std::ifstream input(direct);
    if (input) {
      Json::Value body(Json::objectValue);
      body[pipeline::KEY_CONTENT] = readJsonl(input);
      dates[date] = body;
    }
  }
  const bool scan = date.empty() && std::filesystem::is_directory(directory);
  if (scan) {
    const std::string prefix = std::string(pipeline::LOG_API_PAGE) + "_";
    const std::string suffix = pipeline::LOG_API_SUFFIX;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(directory)) {
      const std::string filename = entry.path().filename().string();
      const bool shaped = entry.is_regular_file() &&
                          filename.size() == prefix.size() + pipeline::DATE_LENGTH + suffix.size() &&
                          filename.compare(0, prefix.size(), prefix) == 0 &&
                          filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0;
      const std::string stamp = shaped ? filename.substr(prefix.size(), pipeline::DATE_LENGTH) : "";
      if (shaped && queryDate(stamp)) {
        std::ifstream file(entry.path());
        Json::Value body(Json::objectValue);
        body[pipeline::KEY_CONTENT] = readJsonl(file);
        dates[stamp] = body;
      }
    }
  }
  return dates;
}

Json::Value PipelineService::getGstreamerLogs(const std::string& name, const std::string& date) {
  const std::filesystem::path path =
      std::filesystem::path(pipeline::DEEPSTREAM_LOG_DIR) / name / pipeline::LOG_GSTREAMER_DIR /
      (std::string(pipeline::LOG_GSTREAMER_PREFIX) + date + pipeline::LOG_GSTREAMER_SUFFIX);
  std::ifstream input(path);
  std::ostringstream text;
  text << input.rdbuf();
  Json::Value body(Json::objectValue);
  body[pipeline::KEY_CONTENT] = input ? text.str() : "";
  return body;
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
    node.SetTag(pipeline::YAML_STRING_TAG);
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
                                        const Json::Value& input) {
  Json::Value data = pipeline::ROW();
  data[pipeline::KEY_NAME] = name.isNull() ? data[pipeline::KEY_NAME] : name;
  data[pipeline::KEY_TYPE] = type.isNull() ? data[pipeline::KEY_TYPE] : type;
  data[pipeline::KEY_STATUS] = status.isNull() ? data[pipeline::KEY_STATUS] : status;
  data[pipeline::KEY_INPUT] = input.isNull() ? data[pipeline::KEY_INPUT] : input;
  data[pipeline::KEY_CONFIG_PATH] = configPath.isNull() ? data[pipeline::KEY_CONFIG_PATH] : configPath;
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const auto rows = Sql::sync({}, dbClient, 
      "INSERT INTO pipeline (id, name, type, status, input, config_path, updated_at) "
      "VALUES (gen_random_uuid(), $1, $2, $3, $4::jsonb, NULLIF($5, ''), now()) " +
      std::string("RETURNING id::text AS id, name, type, status, input::text AS input, config_path, ") +
      pipeline::SQL_REF_COLUMNS +
      "start_time, end_time, duration, updated_at",
      data[pipeline::KEY_NAME].asString(),
      data[pipeline::KEY_TYPE].asString(),
      data[pipeline::KEY_STATUS].asString(),
      Json::writeString(builder, data[pipeline::KEY_INPUT]),
      data[pipeline::KEY_CONFIG_PATH].asString());
  if (rows.empty()) {
    throw std::runtime_error("pipeline not created");
  }
  Json::Value result = row2json(rows[0]);
  return result;
}

void PipelineService::expandGie(Json::Value& input) {
  const char* slots[] = {pipeline::KEY_PGIE, pipeline::KEY_SGIE, pipeline::KEY_RTMPOSE, pipeline::KEY_STGCNPP};
  for (const char* slot : slots) {
    const bool present = input.isMember(slot) && input[slot].isString();
    if (present) {
      const bool primary = std::string(slot) == pipeline::KEY_PGIE;
      const auto rows = Sql::sync({}, dbClient,
          "SELECT m.engine_path, g.interval, g.class_attrs::text AS class_attrs "
          "FROM pipeline_gie g JOIN model m ON m.id = g.model_id WHERE g.id = $1::uuid",
          input[slot].asString());
      if (rows.empty()) {
        throw std::runtime_error("gie not found");
      }
      Json::Value body;
      body[pipeline::KEY_MODEL_ZIP] = rows[0]["engine_path"].isNull() ? "" : rows[0]["engine_path"].as<std::string>();
      body[pipeline_gie::KEY_INTERVAL] = rows[0]["interval"].as<int>();
      Json::Value attrs;
      if (primary && Json::Reader().parse(rows[0]["class_attrs"].as<std::string>(), attrs)) {
        body[pipeline_gie::KEY_CLASS_ATTRS] = attrs;
      }
      input[slot] = body;
    }
  }
}

void PipelineService::expandAnalyzer(Json::Value& source, int index, Json::Value& analyzer) {
  const bool marked = source.isObject() && source.isMember(pipeline::KEY_ANALYZER) && source[pipeline::KEY_ANALYZER].isString();
  if (marked) {
    const auto rows = Sql::sync({}, dbClient,
        "SELECT annotations::text AS annotations FROM pipeline_analyzer WHERE id = $1::uuid",
        source[pipeline::KEY_ANALYZER].asString());
    if (rows.empty()) {
      throw std::runtime_error("analyzer not found");
    }
    Json::Value annotations;
    Json::Reader().parse(rows[0]["annotations"].as<std::string>(), annotations);
    const std::string suffix = pipeline::ANALYZER_STREAM_SUFFIX;
    const auto keys = annotations.isObject() ? annotations.getMemberNames() : std::vector<std::string>();
    for (const auto& key : keys) {
      const bool stream = key.size() >= suffix.size() &&
          key.compare(key.size() - suffix.size(), suffix.size(), suffix) == 0;
      analyzer[stream ? key + "-" + std::to_string(index) : key] = annotations[key];
    }
  }
  if (source.isObject()) {
    source.removeMember(pipeline::KEY_ANALYZER);
  }
}

void PipelineService::expandStream(Json::Value& input, Json::Value& analyzer) {
  const bool listed = input.isMember(pipeline::KEY_STREAMS) && input[pipeline::KEY_STREAMS].isArray();
  if (listed) {
    Json::Value streams(Json::objectValue);
    int index = 0;
    for (auto& item : input[pipeline::KEY_STREAMS]) {
      expandAnalyzer(item, index, analyzer);
      const std::string id = item.isObject() && item.isMember(pipeline::KEY_ID) ? item[pipeline::KEY_ID].asString() : "";
      const auto rows = Sql::sync({}, dbClient,
          "SELECT name, url, width, height, fps FROM stream WHERE id = $1::uuid",
          id);
      if (rows.empty()) {
        throw std::runtime_error("stream not found");
      }
      Json::Value source;
      source[pipeline::KEY_URL] = rows[0]["url"].as<std::string>();
      source[pipeline::KEY_WIDTH] = rows[0]["width"].isNull() ? Json::Value() : rows[0]["width"].as<int>();
      source[pipeline::KEY_HEIGHT] = rows[0]["height"].isNull() ? Json::Value() : rows[0]["height"].as<int>();
      source[pipeline::KEY_FPS] = rows[0]["fps"].isNull() ? Json::Value() : rows[0]["fps"].as<double>();
      streams[rows[0]["name"].as<std::string>()] = source;
      index = index + 1;
    }
    input[pipeline::KEY_STREAMS] = streams;
  }
}

void PipelineService::expandMedia(Json::Value& input, Json::Value& analyzer) {
  const char* lists[] = {pipeline::KEY_VIDEOS, pipeline::KEY_IMAGES};
  for (const char* key : lists) {
    if (input.isMember(key) && input[key].isArray()) {
      Json::Value paths(Json::arrayValue);
      int index = 0;
      for (auto& item : input[key]) {
        expandAnalyzer(item, index, analyzer);
        const bool referenced = item.isObject() && item.isMember(pipeline::KEY_ID);
        if (referenced) {
          const auto rows = Sql::sync({}, dbClient,
              "SELECT name, folder, width, height, fps FROM media WHERE id = $1::uuid",
              item[pipeline::KEY_ID].asString());
          if (rows.empty()) {
            throw std::runtime_error("media not found");
          }
          std::string folder = rows[0]["folder"].as<std::string>();
          if (!folder.empty() && folder.back() == '/') {
            folder.pop_back();
          }
          Json::Value source(Json::objectValue);
          source[pipeline::KEY_PATH] = folder + "/" + rows[0]["name"].as<std::string>();
          source[pipeline::KEY_WIDTH] = rows[0]["width"].isNull() ? Json::Value() : rows[0]["width"].as<int>();
          source[pipeline::KEY_HEIGHT] = rows[0]["height"].isNull() ? Json::Value() : rows[0]["height"].as<int>();
          const bool video = std::string(key) == pipeline::KEY_VIDEOS;
          if (video) {
            source[pipeline::KEY_FPS] = rows[0]["fps"].isNull() ? Json::Value() : rows[0]["fps"].as<double>();
          }
          paths.append(source);
        }
        if (!referenced) {
          paths.append(item);
        }
        index = index + 1;
      }
      input[key] = paths;
    }
  }
}

Json::Value PipelineService::assembleInput(Json::Value input) {
  Json::Value analyzer(Json::objectValue);
  if (input.isObject()) {
    expandGie(input);
    expandStream(input, analyzer);
    expandMedia(input, analyzer);
    input[pipeline::KEY_ANALYZER] = analyzer.empty() ? Json::Value() : analyzer;
  }
  return input;
}

void PipelineService::removeArtifact(const drogon::orm::Row& row) {
  const std::string name = row["name"].as<std::string>();
  // /root/logs/deepstream/{name}
  std::filesystem::remove_all(std::filesystem::path(pipeline::DEEPSTREAM_LOG_DIR) / name);
  // /root/events/{name}
  std::filesystem::remove_all(std::filesystem::path(pipeline::EVENT_DIR) / name);
  // /root/media/pipeline/{name}-{timestamp}-{suffix}
  const std::filesystem::path mediaRoot(pipeline::FILESINK_DIR);
  const std::string prefix = name + "-";
  const bool hasMedia = std::filesystem::exists(mediaRoot);
  if (hasMedia) {
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(mediaRoot)) {
      const bool matched = entry.is_directory() && entry.path().filename().string().rfind(prefix, 0) == 0;
      if (matched) {
        std::filesystem::remove_all(entry.path());
      }
    }
  }
  // config zip at config_path
  const bool hasPath = !row["config_path"].isNull();
  if (hasPath) {
    std::filesystem::remove(row["config_path"].as<std::string>());
  }
}

bool PipelineService::reserveOutputDirectory(const std::filesystem::path& path) {
  std::filesystem::create_directories(path.parent_path());
  const bool created = std::filesystem::create_directory(path);
  return created;
}

Json::Value PipelineService::addOutput(Json::Value input, const std::string& name) {
  const std::string generator = input.isMember(pipeline::KEY_GENERATOR) && input[pipeline::KEY_GENERATOR].isString()
      ? input[pipeline::KEY_GENERATOR].asString()
      : "";
  const bool filesink = input.isMember(pipeline::KEY_IMAGES)
      || (input.isMember(pipeline::KEY_VIDEOS) && generator.find(pipeline::VIS_GENERATOR_MARK) != std::string::npos);
  const bool capturer = input.isMember(pipeline::KEY_EVENT_CODER) && !input[pipeline::KEY_EVENT_CODER].isNull();
  const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm local{};
  localtime_r(&now, &local);
  std::ostringstream text;
  text << std::put_time(&local, "%Y%m%d-%H%M%S");
  const std::string stamp = text.str();
  int suffix = 0;
  bool reserved = !filesink && !capturer;
  std::string filesinkPath;
  std::string capturerPath;
  for (; suffix < pipeline::OUTPUT_SUFFIX_LIMIT && !reserved; ++suffix) {
    const std::string leaf = name + "-" + stamp + "-" + std::to_string(suffix);
    filesinkPath = (std::filesystem::path(pipeline::FILESINK_DIR) / leaf).string();
    capturerPath = (std::filesystem::path(pipeline::EVENT_DIR) / name / (stamp + "-" + std::to_string(suffix))).string();
    const bool filesinkReady = !filesink || reserveOutputDirectory(filesinkPath);
    const bool capturerReady = !capturer || reserveOutputDirectory(capturerPath);
    if (filesink && filesinkReady && !capturerReady) {
      std::filesystem::remove(filesinkPath);
    }
    if (capturer && capturerReady && !filesinkReady) {
      std::filesystem::remove(capturerPath);
    }
    reserved = filesinkReady && capturerReady;
  }
  if (!reserved) {
    throw std::runtime_error("output directory not reserved");
  }
  if (filesink) {
    input[pipeline::KEY_FILESINK_OUTPUT] = filesinkPath;
  }
  if (capturer) {
    if (!input.isMember(pipeline::KEY_COLLECTOR) || !input[pipeline::KEY_COLLECTOR].isObject()) {
      input[pipeline::KEY_COLLECTOR] = Json::objectValue;
    }
    input[pipeline::KEY_COLLECTOR][pipeline::KEY_CAPTURER_OUTPUT] = capturerPath;
  }
  return input;
}

Json::Value PipelineService::setConfigPath(const std::string& pipelineId, const std::string& configPath) {
  const Json::Value current = get(pipelineId);
  const bool hasPath = current[pipeline::KEY_CONFIG_PATH].isString();
  const std::string oldPath = hasPath ? current[pipeline::KEY_CONFIG_PATH].asString() : "";
  const auto rows = Sql::sync({}, dbClient,
      "UPDATE pipeline SET config_path = NULLIF($2, '') WHERE id = $1::uuid " +
      std::string("RETURNING id::text AS id, name, type, status, input::text AS input, config_path, ") +
      pipeline::SQL_REF_COLUMNS +
      "start_time, end_time, duration, updated_at",
      pipelineId,
      configPath);
  if (rows.empty()) {
    throw std::runtime_error("pipeline not updated");
  }
  if (hasPath && oldPath != configPath) {
    std::filesystem::remove(oldPath);
  }
  Json::Value result = row2json(rows[0]);
  return result;
}

Json::Value PipelineService::update(const std::string& pipelineId,
                                        const Json::Value& status,
                                        const Json::Value& configPath,
                                        const Json::Value& name,
                                        const Json::Value& type,
                                        const Json::Value& input) {
  const Json::Value current = get(pipelineId);
  const bool hasPath = current[pipeline::KEY_CONFIG_PATH].isString();
  const std::string oldPath = hasPath ? current[pipeline::KEY_CONFIG_PATH].asString() : "";
  Json::StreamWriterBuilder builder;
  builder["indentation"] = "";
  const auto rows = Sql::sync({}, dbClient, 
      "UPDATE pipeline SET name = $2, type = $3, status = $4, input = $5::jsonb, config_path = NULLIF($6, ''), "
      "start_time = NULL, end_time = NULL, duration = NULL, updated_at = now() "
      "WHERE id = $1::uuid AND status <> $7 " +
      std::string("RETURNING id::text AS id, name, type, status, input::text AS input, config_path, ") +
      pipeline::SQL_REF_COLUMNS +
      "start_time, end_time, duration, updated_at",
      pipelineId,
      name.asString(),
      type.asString(),
      status.asString(),
      Json::writeString(builder, input),
      configPath.asString(),
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
  removeArtifact(rows[0]);
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
        const bool mapped = item.IsMap() && item[pipeline::KEY_PATH];
        if (scalar) {
          paths.push_back(item.as<std::string>());
        }
        if (mapped) {
          paths.push_back(item[pipeline::KEY_PATH].as<std::string>());
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
  node[pipeline::KEY_MEDIA_IDS] = json2array(row["media_ids"].as<std::string>());
  node[pipeline::KEY_MEDIA_NAMES] = json2array(row["media_names"].as<std::string>());
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
