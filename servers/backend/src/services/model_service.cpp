#include "services/model_service.hpp"

#include "constants/model_const.hpp"
#include "searcher/searcher.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>

#include <sys/wait.h>
#include <zip.h>

ModelService::ModelService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

Json::Value ModelService::list(const std::string& search, int page, int pageSize) {
  const int offset = (page - 1) * pageSize;
  const std::string columns =
      "id::text AS id, name, type, version, batch_size, batch_mode, input_tensors::text AS input_tensors, output_tensors::text AS output_tensors, precision, "
      "opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, status, built_at, trt_version, ever_built ";
  const std::string match = Searcher::clause(
      {"id",
       "name",
       "type",
       "version",
       "batch_size",
       "batch_mode",
       "input_tensors",
       "output_tensors",
       "precision",
       "opt_level",
       "device",
       "workspace",
       "static_plugin",
       "max_det",
       "conf",
       "iou",
       "num_class",
       "classes",
       "source_path",
       "status",
       "built_at",
       "trt_version"},
      1);
  const std::string from = "FROM model WHERE " + match + " ";
  const auto counted = dbClient->execSqlSync("SELECT count(*) AS total " + from, search);
  const auto rows = dbClient->execSqlSync(
      "SELECT " + columns + from + "ORDER BY name LIMIT $2 OFFSET $3", search, pageSize, offset);
  Json::Value items(Json::arrayValue);
  for (const auto& row : rows) {
    items.append(row2json(row));
  }
  Json::Value data;
  data[model::KEY_ITEMS] = items;
  data[model::KEY_TOTAL] = counted[0]["total"].as<long>();
  data[model::KEY_PAGE] = page;
  data[model::KEY_PAGE_SIZE] = pageSize;
  return data;
}

Json::Value ModelService::get(const std::string& modelId) {
  const auto rows = dbClient->execSqlSync(
      "SELECT id::text AS id, name, type, version, batch_size, batch_mode, input_tensors::text AS input_tensors, output_tensors::text AS output_tensors, "
      "precision, opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, status, built_at, trt_version, ever_built "
      "FROM model WHERE id = $1::uuid",
      modelId);
  if (rows.empty()) {
    throw std::runtime_error("model not found");
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value ModelService::remove(const std::string& modelId) {
  const auto used = dbClient->execSqlSync(
      "SELECT id FROM pipeline_gie WHERE model_id = $1::uuid LIMIT 1", modelId);
  if (!used.empty()) {
    throw std::runtime_error(model::ERR_OCCUPIED);
  }
  const auto rows = dbClient->execSqlSync(
      "DELETE FROM model WHERE id = $1::uuid AND status <> $2 "
      "RETURNING id::text AS id, name, type, version, batch_size, batch_mode, "
      "input_tensors::text AS input_tensors, output_tensors::text AS output_tensors, "
      "precision, opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, engine_path, status, built_at, trt_version, ever_built",
      modelId,
      model::STATUS_BUILDING);
  if (rows.empty()) {
    const Json::Value current = get(modelId);
    const bool building = current[model::KEY_STATUS].asString() == model::STATUS_BUILDING;
    throw std::runtime_error(building ? model::ERR_BUILDING : "model not removed");
  }
  const bool hasSource = !rows[0]["source_path"].isNull();
  const bool hasEngine = !rows[0]["engine_path"].isNull();
  if (hasSource) {
    std::filesystem::remove(rows[0]["source_path"].as<std::string>());
  }
  if (hasEngine) {
    std::filesystem::remove(rows[0]["engine_path"].as<std::string>());
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value ModelService::update(const std::string& modelId,
                                  const Json::Value& nameValue,
                                  const Json::Value& typeValue,
                                  const Json::Value& batchSizeValue,
                                  const Json::Value& batchModeValue,
                                  const Json::Value& precisionValue,
                                  const Json::Value& optLevelValue,
                                  const Json::Value& deviceValue,
                                  const Json::Value& maxDetValue,
                                  const Json::Value& confValue,
                                  const Json::Value& iouValue) {
  const Json::Value current = get(modelId);
  const Json::Value name = nameValue.isNull() ? current[model::KEY_NAME] : nameValue;
  const Json::Value type = typeValue.isNull() ? current[model::KEY_TYPE] : typeValue;
  const Json::Value batchSize = batchSizeValue.isNull() ? current[model::KEY_BATCH_SIZE] : batchSizeValue;
  const Json::Value batchMode = batchModeValue.isNull() ? current[model::KEY_BATCH_MODE] : batchModeValue;
  const Json::Value precision = precisionValue.isNull() ? current[model::KEY_PRECISION] : precisionValue;
  const Json::Value optLevel = optLevelValue.isNull() ? current[model::KEY_OPT_LEVEL] : optLevelValue;
  const Json::Value device = deviceValue.isNull() ? current[model::KEY_DEVICE] : deviceValue;
  const Json::Value maxDet = maxDetValue.isNull() ? current[model::KEY_MAX_DET] : maxDetValue;
  const Json::Value conf = confValue.isNull() ? current[model::KEY_CONF] : confValue;
  const Json::Value iou = iouValue.isNull() ? current[model::KEY_IOU] : iouValue;
  const bool changed = type != current[model::KEY_TYPE] || batchSize != current[model::KEY_BATCH_SIZE] ||
      batchMode != current[model::KEY_BATCH_MODE] || precision != current[model::KEY_PRECISION] ||
      optLevel != current[model::KEY_OPT_LEVEL] || device != current[model::KEY_DEVICE] ||
      maxDet != current[model::KEY_MAX_DET] || conf != current[model::KEY_CONF] || iou != current[model::KEY_IOU];
  const bool building = current[model::KEY_STATUS].asString() == model::STATUS_BUILDING;
  const bool built = current[model::KEY_STATUS].asString() == model::STATUS_BUILT;
  if ((building || built) && changed) {
    throw std::runtime_error(building ? model::ERR_BUILDING : model::ERR_BUILT);
  }
  if (!createCheck(name.asString(), type.asString(), batchSize.asInt(), batchMode.asString(), precision.asString())) {
    throw std::runtime_error("invalid model");
  }
  Json::Value row = current;
  row[model::KEY_NAME] = name;
  row[model::KEY_TYPE] = type;
  row[model::KEY_BATCH_SIZE] = batchSize;
  row[model::KEY_BATCH_MODE] = batchMode;
  row[model::KEY_PRECISION] = precision;
  row[model::KEY_OPT_LEVEL] = optLevel;
  row[model::KEY_DEVICE] = device;
  row[model::KEY_MAX_DET] = maxDet;
  row[model::KEY_CONF] = conf;
  row[model::KEY_IOU] = iou;
  row[model::KEY_VERSION] = changed ? Json::Value() : current[model::KEY_VERSION];
  row[model::KEY_NUM_CLASS] = changed ? Json::Value() : current[model::KEY_NUM_CLASS];
  row[model::KEY_CLASSES] = changed ? Json::Value() : current[model::KEY_CLASSES];
  row[model::KEY_BUILT_AT] = changed ? Json::Value() : current[model::KEY_BUILT_AT];
  const char* columns =
      "id::text AS id, name, type, version, batch_size, batch_mode, input_tensors::text AS input_tensors, output_tensors::text AS output_tensors, precision, "
      "opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, status, built_at, trt_version, ever_built";
  const auto rows = dbClient->execSqlSync(
      std::string("UPDATE model SET name = $1, type = $2, batch_size = $3, batch_mode = $4, precision = $5, "
                  "source_path = COALESCE(NULLIF($6, ''), source_path), "
                  "version = CASE WHEN $7 THEN NULL ELSE version END, "
                  "num_class = CASE WHEN $7 THEN NULL ELSE num_class END, "
                  "classes = CASE WHEN $7 THEN NULL ELSE classes END, "
                  "built_at = CASE WHEN $7 THEN NULL ELSE built_at END, "
                  "engine_path = CASE WHEN $7 THEN NULL ELSE engine_path END, "
                  "trt_version = CASE WHEN $7 THEN NULL ELSE trt_version END, "
                  "status = $8, max_det = NULLIF($10, '')::int, conf = NULLIF($11, '')::numeric(4, 2), "
                  "iou = NULLIF($12, '')::numeric(4, 2), "
                  "input_tensors = NULLIF($13, '')::jsonb, output_tensors = NULLIF($14, '')::jsonb, "
                  "opt_level = NULLIF($15, '')::int, device = $16, workspace = $17, static_plugin = NULLIF($18, '') "
                  "WHERE id = $9::uuid RETURNING ") +
          columns,
      row[model::KEY_NAME].asString(),
      row[model::KEY_TYPE].asString(),
      row[model::KEY_BATCH_SIZE].asInt(),
      row[model::KEY_BATCH_MODE].asString(),
      row[model::KEY_PRECISION].asString(),
      row[model::KEY_SOURCE_PATH].isString() ? row[model::KEY_SOURCE_PATH].asString() : "",
      changed,
      row[model::KEY_STATUS].asString(),
      row[model::KEY_ID].asString(),
      row[model::KEY_MAX_DET].isNull() ? "" : std::to_string(row[model::KEY_MAX_DET].asInt()),
      row[model::KEY_CONF].isNull() ? "" : row[model::KEY_CONF].asString(),
      row[model::KEY_IOU].isNull() ? "" : row[model::KEY_IOU].asString(),
      row[model::KEY_INPUT_TENSORS].isNull() ? "" : Json::writeString(Json::StreamWriterBuilder(), row[model::KEY_INPUT_TENSORS]),
      row[model::KEY_OUTPUT_TENSORS].isNull() ? "" : Json::writeString(Json::StreamWriterBuilder(), row[model::KEY_OUTPUT_TENSORS]),
      row[model::KEY_OPT_LEVEL].isNull() ? "" : std::to_string(row[model::KEY_OPT_LEVEL].asInt()),
      row[model::KEY_DEVICE].asInt(),
      row[model::KEY_WORKSPACE].asInt(),
      row[model::KEY_STATIC_PLUGIN].isNull() ? "" : row[model::KEY_STATIC_PLUGIN].asString());
  if (rows.empty()) {
    throw std::runtime_error("model not updated");
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

BuildStart ModelService::buildStart(const std::string& modelId) {
  const Json::Value current = get(modelId);
  const auto recipe = model::RECIPES.find(current[model::KEY_TYPE].asString());
  if (recipe == model::RECIPES.end()) {
    throw std::runtime_error("model recipe missing");
  }
  Json::Value sections;
  for (const auto& section : recipe->second) {
    Json::Value fields;
    for (const auto& field : section.second) {
      fields[field.first] = field.second;
    }
    sections[section.first] = fields;
  }
  const std::string batchSize = std::to_string(current[model::KEY_BATCH_SIZE].asInt());
  const std::string batchMode = current[model::KEY_BATCH_MODE].asString();
  const std::string precision = current[model::KEY_PRECISION].asString();
  for (const auto& name : sections.getMemberNames()) {
    sections[name][model::KEY_BATCH_SIZE] = batchSize;
    sections[name][model::KEY_BATCH_MODE] = batchMode;
    sections[name][model::KEY_PRECISION] = precision;
    if (!current[model::KEY_MAX_DET].isNull()) {
      sections[name][model::KEY_MAX_DET] = std::to_string(current[model::KEY_MAX_DET].asInt());
    }
    if (!current[model::KEY_CONF].isNull()) {
      sections[name][model::KEY_CONF] = current[model::KEY_CONF].asString();
    }
    if (!current[model::KEY_IOU].isNull()) {
      sections[name][model::KEY_IOU] = current[model::KEY_IOU].asString();
    }
  }
  const auto rows = dbClient->execSqlSync(
      "UPDATE model SET status = $2 WHERE id = $1::uuid AND status <> $2 AND source_path IS NOT NULL "
      "RETURNING id",
      modelId,
      model::STATUS_BUILDING);
  if (rows.empty()) {
    const bool building = current[model::KEY_STATUS].asString() == model::STATUS_BUILDING;
    throw std::runtime_error(building ? model::ERR_BUILDING : "model source missing");
  }
  BuildStart data;
  data.exportOnnx = sections.isMember(model::SECTION_EXPORT_ONNX) ? json2yaml(sections[model::SECTION_EXPORT_ONNX]) : "";
  data.exportTrt = sections.isMember(model::SECTION_EXPORT_TRT) ? json2yaml(sections[model::SECTION_EXPORT_TRT]) : "";
  return data;
}

ExportResponse ModelService::buildParse(const std::string& contentType, const std::string& body) {
  const std::string marker = "boundary=";
  const std::size_t at = contentType.find(marker);
  std::string boundary = at == std::string::npos ? "" : contentType.substr(at + marker.size());
  const std::size_t semicolon = boundary.find(';');
  boundary = semicolon == std::string::npos ? boundary : boundary.substr(0, semicolon);
  const bool quoted = boundary.size() >= 2 && boundary.front() == '"';
  boundary = quoted ? boundary.substr(1, boundary.size() - 2) : boundary;
  const std::string delimiter = "--" + boundary;
  ExportResponse parsed;
  bool success = false;
  bool sawData = false;
  bool sawFile = false;
  std::size_t cursor = 0;
  while (!boundary.empty() && cursor < body.size()) {
    const std::size_t start = body.find(delimiter, cursor);
    const std::size_t after = start == std::string::npos ? body.size() : start + delimiter.size();
    const bool closing = after + 1 < body.size() && body.compare(after, 2, "--") == 0;
    const std::size_t headerStart = start == std::string::npos ? std::string::npos : body.find("\r\n", after);
    const std::size_t next = start == std::string::npos ? std::string::npos : body.find(delimiter, after);
    const bool part = !closing && headerStart != std::string::npos && next != std::string::npos;
    const std::string section = part ? body.substr(headerStart + 2, next - headerStart - 2) : "";
    const std::size_t split = section.find("\r\n\r\n");
    const std::string headers = split == std::string::npos ? "" : section.substr(0, split);
    std::string content = split == std::string::npos ? "" : section.substr(split + 4);
    const bool crlf = content.size() >= 2 && content.ends_with("\r\n");
    content = crlf ? content.substr(0, content.size() - 2) : content;
    const std::size_t nameAt = headers.find("name=\"");
    const std::size_t nameEnd = nameAt == std::string::npos ? std::string::npos : headers.find('"', nameAt + 6);
    const std::string name = nameEnd == std::string::npos ? "" : headers.substr(nameAt + 6, nameEnd - nameAt - 6);
    const std::size_t fileAt = headers.find("filename=\"");
    const std::size_t fileEnd = fileAt == std::string::npos ? std::string::npos : headers.find('"', fileAt + 10);
    const std::string filename = fileEnd == std::string::npos ? "" : headers.substr(fileAt + 10, fileEnd - fileAt - 10);
    success = name == "success" ? content == "true" : success;
    sawData = name == "data" ? Json::Reader().parse(content, parsed.data) : sawData;
    parsed.fileName = name == model::FIELD_FILE ? std::filesystem::path(filename).filename().string() : parsed.fileName;
    parsed.fileContent = name == model::FIELD_FILE ? content : parsed.fileContent;
    sawFile = name == model::FIELD_FILE ? !parsed.fileName.empty() && !parsed.fileContent.empty() : sawFile;
    cursor = next == std::string::npos || closing || !part ? body.size() : next;
  }
  const bool accepted = success && sawData && sawFile;
  if (!accepted) {
    throw std::runtime_error("export failed");
  }
  return parsed;
}

Json::Value ModelService::buildEnd(const std::string& modelId,
                                     const Json::Value& data,
                                     const std::string& fileName,
                                     const std::string& fileContent) {
  const char* columns =
      "id::text AS id, name, type, version, batch_size, batch_mode, input_tensors::text AS input_tensors, "
      "output_tensors::text AS output_tensors, precision, opt_level, device, workspace, static_plugin, max_det, "
      "conf, iou, num_class, classes::text AS classes, source_path, status, built_at, trt_version, ever_built";
  Json::Value row;
  if (data.isNull()) {
    const auto rows = dbClient->execSqlSync(
        std::string("UPDATE model SET status = CASE WHEN ever_built THEN $2 ELSE $3 END WHERE id = $1::uuid "
                    "RETURNING ") +
            columns,
        modelId,
        model::STATUS_BUILT,
        model::STATUS_NOT_BUILT);
    if (rows.empty()) {
      throw std::runtime_error("model not ended");
    }
    row = row2json(rows[0]);
  } else {
    const std::filesystem::path engine = std::filesystem::path(model::ENGINE_DIR) / std::filesystem::path(fileName).filename();
    std::filesystem::create_directories(model::ENGINE_DIR);
    std::ofstream output(engine, std::ios::binary);
    output.write(fileContent.data(), static_cast<std::streamsize>(fileContent.size()));
    output.close();
    if (!output || fileContent.empty()) {
      throw std::runtime_error("engine file not saved");
    }
    const Json::Value& inputTensors = data[model::KEY_INPUT_TENSORS];
    const Json::Value& outputTensors = data[model::KEY_OUTPUT_TENSORS];
    const Json::Value& optLevel = data[model::KEY_OPT_LEVEL];
    const Json::Value& staticPlugin = data[model::KEY_STATIC_PLUGIN];
    const Json::Value& maxDet = data[model::KEY_MAX_DET];
    const Json::Value& conf = data[model::KEY_CONF];
    const Json::Value& iou = data[model::KEY_IOU];
    const Json::Value& numClass = data[model::KEY_NUM_CLASS];
    const Json::Value& classes = data[model::KEY_CLASSES];
    const Json::Value& builtAt = data[model::KEY_BUILT_AT];
    const Json::Value& trtVersion = data[model::KEY_TRT_VERSION];
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const auto rows = dbClient->execSqlSync(
        std::string("UPDATE model SET type = $2, batch_size = $3, batch_mode = $4, input_tensors = $5::jsonb, "
                    "output_tensors = $6::jsonb, precision = $7, opt_level = $8, device = $9, workspace = $10, "
                    "static_plugin = $11, max_det = $12, conf = $13, iou = $14, num_class = $15, classes = $16::jsonb, "
                    "engine_path = $17, status = $18, built_at = $19::timestamptz, trt_version = $20, "
                    "ever_built = TRUE WHERE id = $1::uuid RETURNING ") +
            columns,
        modelId,
        data[model::KEY_TYPE].asString(),
        data[model::KEY_BATCH_SIZE].asInt(),
        data[model::KEY_BATCH_MODE].asString(),
        inputTensors.isNull() ? std::nullopt : std::optional<std::string>(Json::writeString(builder, inputTensors)),
        outputTensors.isNull() ? std::nullopt : std::optional<std::string>(Json::writeString(builder, outputTensors)),
        data[model::KEY_PRECISION].asString(),
        optLevel.isNull() ? std::nullopt : std::optional<int>(optLevel.asInt()),
        data[model::KEY_DEVICE].asInt(),
        data[model::KEY_WORKSPACE].asInt(),
        staticPlugin.isNull() ? std::nullopt : std::optional<std::string>(staticPlugin.asString()),
        maxDet.isNull() ? std::nullopt : std::optional<int>(maxDet.asInt()),
        conf.isNull() ? std::nullopt : std::optional<double>(conf.asDouble()),
        iou.isNull() ? std::nullopt : std::optional<double>(iou.asDouble()),
        numClass.isNull() ? std::nullopt : std::optional<int>(numClass.asInt()),
        classes.isNull() ? std::nullopt : std::optional<std::string>(Json::writeString(builder, classes)),
        engine.string(),
        data[model::KEY_STATUS].asString(),
        builtAt.isNull() ? std::nullopt : std::optional<std::string>(builtAt.asString()),
        trtVersion.isNull() ? std::nullopt : std::optional<std::string>(std::to_string(trtVersion.asInt())));
    if (rows.empty()) {
      throw std::runtime_error("model not ended");
    }
    row = row2json(rows[0]);
  }
  return row;
}

bool ModelService::uploadCheck(const std::string& path) {
  const std::string command = "unzip -Z -1 '" + path + "'";
  FILE* pipe = popen(command.c_str(), "r");
  std::string listing;
  char buffer[256];
  while (pipe && std::fgets(buffer, sizeof(buffer), pipe)) {
    listing += buffer;
  }
  const int status = pipe ? pclose(pipe) : -1;
  const bool listed = status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
  std::string onnx;
  std::string data;
  int files = 0;
  std::istringstream lines(listing);
  std::string entry;
  while (listed && std::getline(lines, entry)) {
    if (!entry.empty() && entry.back() == '\r') {
      entry.pop_back();
    }
    const bool directory = !entry.empty() && entry.back() == '/';
    const std::string filename = std::filesystem::path(entry).filename().string();
    const bool dataFile = filename.ends_with(model::SUFFIX_ONNX_DATA);
    const bool onnxFile = !dataFile && filename.ends_with(model::SUFFIX_ONNX);
    files += directory ? 0 : 1;
    onnx = onnxFile ? filename : onnx;
    data = dataFile ? filename : data;
  }
  const bool paired = !onnx.empty() && data == onnx + ".data";
  const bool accepted = listed && files == 2 && paired;
  return accepted;
}

std::string ModelService::json2yaml(const Json::Value& fields) {
  std::string text;
  for (const auto& name : fields.getMemberNames()) {
    text += name + ": " + fields[name].asString() + "\n";
  }
  return text;
}

Json::Value ModelService::map() {
  const auto rows = dbClient->execSqlSync("SELECT id::text AS id, name FROM model ORDER BY name");
  Json::Value data(Json::objectValue);
  for (const auto& row : rows) {
    data[row["name"].as<std::string>()] = row["id"].as<std::string>();
  }
  return data;
}

Json::Value ModelService::upload(const std::string& modelId,
                                  const std::string& fileName,
                                  const std::string& fileContent) {
  const std::filesystem::path file(std::filesystem::path(fileName).filename());
  const std::string name = file.string();
  const bool plain = name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-") ==
      std::string::npos;
  const bool pt = file.extension() == model::SUFFIX_PT;
  const bool onnx = file.extension() == model::SUFFIX_ONNX;
  const bool zip = file.extension() == model::SUFFIX_ZIP;
  if (!plain || (!pt && !onnx && !zip)) {
    throw std::runtime_error("invalid model file");
  }
  const Json::Value current = get(modelId);
  if (current[model::KEY_STATUS].asString() == model::STATUS_BUILDING) {
    throw std::runtime_error(model::ERR_BUILDING);
  }
  const std::filesystem::path staged = std::filesystem::path(model::UPLOAD_DIR) / name;
  const std::filesystem::path source = std::filesystem::path(model::SOURCE_DIR) / name;
  std::filesystem::create_directories(model::SOURCE_DIR);
  std::filesystem::create_directories(model::UPLOAD_DIR);
  std::ofstream output(staged, std::ios::binary);
  output.write(fileContent.data(), static_cast<std::streamsize>(fileContent.size()));
  output.close();
  const bool accepted = output && (!zip || uploadCheck(staged.string()));
  if (!accepted) {
    std::filesystem::remove(staged);
    throw std::runtime_error("invalid model file");
  }
  std::filesystem::rename(staged, source);
  const bool hasSource = !current[model::KEY_SOURCE_PATH].isNull();
  const std::string oldSource = hasSource ? current[model::KEY_SOURCE_PATH].asString() : "";
  if (hasSource && oldSource != source.string()) {
    std::filesystem::remove(oldSource);
  }
  const char* columns =
      "id::text AS id, name, type, version, batch_size, batch_mode, input_tensors::text AS input_tensors, output_tensors::text AS output_tensors, precision, "
      "opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, status, built_at, trt_version, ever_built";
  const auto rows = dbClient->execSqlSync(
      std::string("UPDATE model SET source_path = $2 WHERE id = $1::uuid RETURNING ") + columns,
      modelId,
      source.string());
  if (rows.empty()) {
    throw std::runtime_error("model not updated");
  }
  Json::Value data = row2json(rows[0]);
  return data;
}

Json::Value ModelService::importModel(const std::vector<ImportFile>& files) {
  Json::Value data = model::BATCH_DATA();
  const char* columns =
      "id::text AS id, name, type, version, batch_size, batch_mode, input_tensors::text AS input_tensors, output_tensors::text AS output_tensors, precision, "
      "opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, status, built_at, trt_version, ever_built";
  std::filesystem::create_directories(model::ENGINE_DIR);
  for (const ImportFile& file : files) {
    const std::filesystem::path path(std::filesystem::path(file.fileName).filename());
    const std::string filename = path.string();
    const bool plain = filename.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-") ==
        std::string::npos;
    const bool zip = path.extension() == model::SUFFIX_ZIP;
    const std::string name = path.stem().string();
    Json::Value meta;
    const bool listed = plain && zip && importCheck(file.fileContent, meta);
    const bool complete = listed && meta.isMember(model::KEY_TYPE) && meta[model::KEY_TYPE].isString() &&
        meta.isMember(model::KEY_BATCH_SIZE) && meta[model::KEY_BATCH_SIZE].isInt() &&
        meta.isMember(model::KEY_BATCH_MODE) && meta[model::KEY_BATCH_MODE].isString() &&
        meta.isMember(model::KEY_PRECISION) && meta[model::KEY_PRECISION].isString() &&
        meta.isMember(model::KEY_DEVICE) && meta[model::KEY_DEVICE].isInt() &&
        meta.isMember(model::KEY_WORKSPACE) && meta[model::KEY_WORKSPACE].isInt();
    const auto existing = complete
        ? dbClient->execSqlSync(
              std::string("SELECT ") + columns + ", engine_path FROM model WHERE name = $1", name)
        : dbClient->execSqlSync("SELECT 1 WHERE false");
    const bool named = complete && !existing.empty();
    Json::Value current = named ? row2json(existing[0]) : Json::Value();
    const std::string engineName = named && !existing[0]["engine_path"].isNull()
        ? std::filesystem::path(existing[0]["engine_path"].as<std::string>()).filename().string()
        : "";
    const bool identical = named && engineName == filename &&
        current[model::KEY_TYPE] == meta[model::KEY_TYPE] &&
        current[model::KEY_BATCH_SIZE] == meta[model::KEY_BATCH_SIZE] &&
        current[model::KEY_BATCH_MODE] == meta[model::KEY_BATCH_MODE] &&
        current[model::KEY_PRECISION] == meta[model::KEY_PRECISION] &&
        current[model::KEY_DEVICE] == meta[model::KEY_DEVICE] &&
        current[model::KEY_WORKSPACE] == meta[model::KEY_WORKSPACE];
    if (identical) {
      data[model::KEY_SKIPPED_ROWS].append(current);
    }
    if (!complete || (named && !identical)) {
      Json::Value failed = named ? current : Json::Value();
      failed[model::KEY_NAME] = named ? failed[model::KEY_NAME] : filename;
      data[model::KEY_FAILED_ROWS].append(failed);
    }
    const std::filesystem::path engine = std::filesystem::path(model::ENGINE_DIR) / filename;
    bool saved = false;
    if (complete && !named) {
      std::ofstream output(engine, std::ios::binary);
      output.write(file.fileContent.data(), static_cast<std::streamsize>(file.fileContent.size()));
      output.close();
      saved = static_cast<bool>(output);
    }
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    const Json::Value inputTensors = saved && meta.isMember(model::KEY_INPUT_TENSORS) ? meta[model::KEY_INPUT_TENSORS] : Json::Value();
    const Json::Value outputTensors = saved && meta.isMember(model::KEY_OUTPUT_TENSORS) ? meta[model::KEY_OUTPUT_TENSORS] : Json::Value();
    const Json::Value classes = saved && meta.isMember(model::KEY_CLASSES) ? meta[model::KEY_CLASSES] : Json::Value();
    const Json::Value trtVersion = saved && meta.isMember(model::KEY_TRT_VERSION) ? meta[model::KEY_TRT_VERSION] : Json::Value();
    const std::string trtText = trtVersion.isString() ? trtVersion.asString() : trtVersion.isInt() ? std::to_string(trtVersion.asInt()) : "";
    const Json::Value conf = saved && meta.isMember(model::KEY_CONF) ? meta[model::KEY_CONF] : Json::Value();
    const Json::Value iou = saved && meta.isMember(model::KEY_IOU) ? meta[model::KEY_IOU] : Json::Value();
    const std::string confText = conf.isString() ? conf.asString() : conf.isDouble() ? std::to_string(conf.asDouble()) : "";
    const std::string iouText = iou.isString() ? iou.asString() : iou.isDouble() ? std::to_string(iou.asDouble()) : "";
    if (saved) {
      const auto rows = dbClient->execSqlSync(
          std::string("INSERT INTO model (id, name, type, batch_size, batch_mode, input_tensors, output_tensors, "
                      "precision, opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, classes, "
                      "source_path, engine_path, status, built_at, trt_version, ever_built) "
                      "VALUES (gen_random_uuid(), $1, $2, $3, $4, NULLIF($5, '')::jsonb, NULLIF($6, '')::jsonb, $7, NULLIF($8, '')::int, "
                      "$9, $10, NULLIF($11, ''), NULLIF($12, '')::int, NULLIF($13, '')::numeric(4, 2), "
                      "NULLIF($14, '')::numeric(4, 2), NULLIF($15, '')::int, NULLIF($16, '')::jsonb, NULL, $17, $18, "
                      "NULLIF($19, '')::timestamptz, NULLIF($20, ''), TRUE) RETURNING ") +
              columns,
          name,
          meta[model::KEY_TYPE].asString(),
          meta[model::KEY_BATCH_SIZE].asInt(),
          meta[model::KEY_BATCH_MODE].asString(),
          inputTensors.isNull() ? "" : Json::writeString(builder, inputTensors),
          outputTensors.isNull() ? "" : Json::writeString(builder, outputTensors),
          meta[model::KEY_PRECISION].asString(),
          meta.isMember(model::KEY_OPT_LEVEL) && meta[model::KEY_OPT_LEVEL].isInt()
              ? std::to_string(meta[model::KEY_OPT_LEVEL].asInt())
              : "",
          meta[model::KEY_DEVICE].asInt(),
          meta[model::KEY_WORKSPACE].asInt(),
          meta.isMember(model::KEY_STATIC_PLUGIN) && meta[model::KEY_STATIC_PLUGIN].isString()
              ? meta[model::KEY_STATIC_PLUGIN].asString()
              : "",
          meta.isMember(model::KEY_MAX_DET) && meta[model::KEY_MAX_DET].isInt() ? std::to_string(meta[model::KEY_MAX_DET].asInt())
                                                                                : "",
          confText,
          iouText,
          meta.isMember(model::KEY_NUM_CLASS) && meta[model::KEY_NUM_CLASS].isInt()
              ? std::to_string(meta[model::KEY_NUM_CLASS].asInt())
              : "",
          classes.isNull() ? "" : Json::writeString(builder, classes),
          engine.string(),
          meta.isMember(model::KEY_STATUS) && meta[model::KEY_STATUS].isString() ? meta[model::KEY_STATUS].asString()
                                                                                 : std::string(model::STATUS_BUILT),
          meta.isMember(model::KEY_BUILT_AT) && meta[model::KEY_BUILT_AT].isString() ? meta[model::KEY_BUILT_AT].asString() : "",
          trtText);
      if (!rows.empty()) {
        data[model::KEY_UPDATED_ROWS].append(row2json(rows[0]));
      }
    }
  }
  data[model::KEY_UPDATED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_UPDATED_ROWS].size());
  data[model::KEY_FAILED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_FAILED_ROWS].size());
  data[model::KEY_SKIPPED_COUNT] = static_cast<Json::UInt64>(data[model::KEY_SKIPPED_ROWS].size());
  return data;
}

bool ModelService::importCheck(const std::string& bytes, Json::Value& meta) {
  zip_error_t zipError;
  zip_error_init(&zipError);
  zip_source_t* source = zip_source_buffer_create(bytes.data(), bytes.size(), 0, &zipError);
  zip_t* zip = source ? zip_open_from_source(source, ZIP_RDONLY, &zipError) : nullptr;
  bool labels = false;
  bool hasMeta = false;
  int engines = 0;
  std::string metaBytes;
  const zip_int64_t count = zip ? zip_get_num_entries(zip, 0) : 0;
  for (zip_int64_t index = 0; index < count; ++index) {
    const std::string entryName = zip_get_name(zip, index, 0);
    const bool directory = !entryName.empty() && entryName.back() == '/';
    const std::string filename = std::filesystem::path(entryName).filename().string();
    const bool metaFile = !directory && filename == model::FILE_META;
    zip_stat_t stat;
    const int stated = metaFile ? zip_stat_index(zip, index, 0, &stat) : -1;
    const bool sized = stated == 0 && (stat.valid & ZIP_STAT_SIZE) != 0;
    zip_file_t* entry = sized ? zip_fopen_index(zip, index, 0) : nullptr;
    std::string content(sized ? static_cast<std::size_t>(stat.size) : 0, '\0');
    const zip_int64_t read = entry ? zip_fread(entry, content.data(), content.size()) : -1;
    if (entry) {
      zip_fclose(entry);
    }
    labels = labels || (!directory && filename == model::FILE_LABELS);
    engines += !directory && filename.ends_with(model::SUFFIX_ENGINE) ? 1 : 0;
    hasMeta = hasMeta || (metaFile && read == static_cast<zip_int64_t>(content.size()));
    metaBytes = metaFile && read == static_cast<zip_int64_t>(content.size()) ? content : metaBytes;
  }
  if (zip) {
    zip_close(zip);
  }
  zip_error_fini(&zipError);
  const bool accepted = labels && hasMeta && engines == 1 && Json::Reader().parse(metaBytes, meta);
  return accepted;
}

Json::Value ModelService::create(const Json::Value& name,
                                  const Json::Value& type,
                                  const Json::Value& batchSize,
                                  const Json::Value& batchMode,
                                  const Json::Value& precision,
                                  const Json::Value& optLevel,
                                  const Json::Value& device,
                                  const Json::Value& maxDet,
                                  const Json::Value& conf,
                                  const Json::Value& iou) {
  const Json::Value defaults = model::ROW();
  if (!createCheck(name.asString(), type.asString(), batchSize.asInt(), batchMode.asString(), precision.asString())) {
    throw std::runtime_error("invalid model");
  }
  Json::Value data = defaults;
  data[model::KEY_NAME] = name;
  data[model::KEY_TYPE] = type;
  data[model::KEY_BATCH_SIZE] = batchSize;
  data[model::KEY_BATCH_MODE] = batchMode;
  data[model::KEY_PRECISION] = precision;
  data[model::KEY_OPT_LEVEL] = optLevel.isNull() ? defaults[model::KEY_OPT_LEVEL] : optLevel;
  data[model::KEY_DEVICE] = device.isNull() ? defaults[model::KEY_DEVICE] : device;
  data[model::KEY_MAX_DET] = maxDet.isNull() ? defaults[model::KEY_MAX_DET] : maxDet;
  data[model::KEY_CONF] = conf.isNull() ? defaults[model::KEY_CONF] : conf;
  data[model::KEY_IOU] = iou.isNull() ? defaults[model::KEY_IOU] : iou;
  const char* columns =
      "id::text AS id, name, type, version, batch_size, batch_mode, input_tensors::text AS input_tensors, output_tensors::text AS output_tensors, precision, "
      "opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, status, built_at, trt_version, ever_built";
  const auto rows = dbClient->execSqlSync(
      std::string("INSERT INTO model (id, name, type, batch_size, batch_mode, precision, status, source_path, "
                  "max_det, conf, iou, input_tensors, output_tensors, opt_level, device, workspace, "
                  "static_plugin, ever_built) "
                  "VALUES (gen_random_uuid(), $1, $2, $3, $4, $5, $6, NULLIF($7, ''), "
                  "NULLIF($8, '')::int, NULLIF($9, '')::numeric(4, 2), NULLIF($10, '')::numeric(4, 2), "
                  "NULLIF($11, '')::jsonb, NULLIF($12, '')::jsonb, NULLIF($13, '')::int, $14, $15, NULLIF($16, ''), $17) "
                  "RETURNING ") +
          columns,
      data[model::KEY_NAME].asString(),
      data[model::KEY_TYPE].asString(),
      data[model::KEY_BATCH_SIZE].asInt(),
      data[model::KEY_BATCH_MODE].asString(),
      data[model::KEY_PRECISION].asString(),
      data[model::KEY_STATUS].asString(),
      "",
      data[model::KEY_MAX_DET].isNull() ? "" : std::to_string(data[model::KEY_MAX_DET].asInt()),
      data[model::KEY_CONF].isNull() ? "" : data[model::KEY_CONF].asString(),
      data[model::KEY_IOU].isNull() ? "" : data[model::KEY_IOU].asString(),
      data[model::KEY_INPUT_TENSORS].isNull() ? "" : Json::writeString(Json::StreamWriterBuilder(), data[model::KEY_INPUT_TENSORS]),
      data[model::KEY_OUTPUT_TENSORS].isNull() ? "" : Json::writeString(Json::StreamWriterBuilder(), data[model::KEY_OUTPUT_TENSORS]),
      data[model::KEY_OPT_LEVEL].isNull() ? "" : std::to_string(data[model::KEY_OPT_LEVEL].asInt()),
      data[model::KEY_DEVICE].asInt(),
      data[model::KEY_WORKSPACE].asInt(),
      data[model::KEY_STATIC_PLUGIN].isNull() ? "" : data[model::KEY_STATIC_PLUGIN].asString(),
      data[model::KEY_EVER_BUILT].asBool());
  if (rows.empty()) {
    throw std::runtime_error("model not created");
  }
  Json::Value result = row2json(rows[0]);
  return result;
}

bool ModelService::createCheck(const std::string& name,
                            const std::string& type,
                            int batchSize,
                            const std::string& batchMode,
                            const std::string& precision) {
  bool knownType = false;
  for (const auto* item : model::AVAILABLE_TYPES) {
    knownType = knownType || type == item;
  }
  bool knownMode = false;
  for (const auto* item : model::AVAILABLE_BATCH_MODES) {
    knownMode = knownMode || batchMode == item;
  }
  bool knownPrecision = false;
  for (const auto* item : model::AVAILABLE_PRECISIONS) {
    knownPrecision = knownPrecision || precision == item;
  }
  const bool knownSize =
      batchSize >= model::AVAILABLE_BATCH_RANGE[0] && batchSize <= model::AVAILABLE_BATCH_RANGE[1];
  const bool valid = !name.empty() && knownType && knownMode && knownSize && knownPrecision;
  return valid;
}

Json::Value ModelService::row2json(const drogon::orm::Row& row) {
  Json::Value classes;
  Json::Value inputTensors;
  Json::Value outputTensors;
  const bool parsed = !row["classes"].isNull() && Json::Reader().parse(row["classes"].as<std::string>(), classes);
  const bool inputParsed =
      !row["input_tensors"].isNull() && Json::Reader().parse(row["input_tensors"].as<std::string>(), inputTensors);
  const bool outputParsed =
      !row["output_tensors"].isNull() && Json::Reader().parse(row["output_tensors"].as<std::string>(), outputTensors);
  Json::Value node;
  node[model::KEY_ID] = row["id"].as<std::string>();
  node[model::KEY_NAME] = row["name"].as<std::string>();
  node[model::KEY_TYPE] = row["type"].as<std::string>();
  node[model::KEY_VERSION] = row["version"].isNull() ? Json::Value() : row["version"].as<std::string>();
  node[model::KEY_BATCH_SIZE] = row["batch_size"].as<int>();
  node[model::KEY_BATCH_MODE] = row["batch_mode"].as<std::string>();
  node[model::KEY_INPUT_TENSORS] = inputParsed ? inputTensors : Json::Value();
  node[model::KEY_OUTPUT_TENSORS] = outputParsed ? outputTensors : Json::Value();
  node[model::KEY_PRECISION] = row["precision"].as<std::string>();
  node[model::KEY_OPT_LEVEL] = row["opt_level"].isNull() ? Json::Value() : row["opt_level"].as<int>();
  node[model::KEY_DEVICE] = row["device"].as<int>();
  node[model::KEY_WORKSPACE] = row["workspace"].as<int>();
  node[model::KEY_STATIC_PLUGIN] =
      row["static_plugin"].isNull() ? Json::Value() : row["static_plugin"].as<std::string>();
  node[model::KEY_MAX_DET] = row["max_det"].isNull() ? Json::Value() : row["max_det"].as<int>();
  node[model::KEY_CONF] = row["conf"].isNull() ? Json::Value() : row["conf"].as<std::string>();
  node[model::KEY_IOU] = row["iou"].isNull() ? Json::Value() : row["iou"].as<std::string>();
  node[model::KEY_NUM_CLASS] = row["num_class"].isNull() ? Json::Value() : row["num_class"].as<int>();
  node[model::KEY_CLASSES] = parsed ? classes : Json::Value();
  node[model::KEY_SOURCE_PATH] =
      row["source_path"].isNull() ? Json::Value() : row["source_path"].as<std::string>();
  node[model::KEY_STATUS] = row["status"].as<std::string>();
  node[model::KEY_BUILT_AT] = row["built_at"].isNull() ? Json::Value() : row["built_at"].as<std::string>();
  node[model::KEY_TRT_VERSION] = row["trt_version"].isNull() ? Json::Value() : row["trt_version"].as<std::string>();
  node[model::KEY_EVER_BUILT] = row["ever_built"].as<bool>();
  return node;
}
