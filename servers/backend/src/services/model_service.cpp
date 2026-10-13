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

ModelService::ModelService(drogon::orm::DbClientPtr dbClient) : dbClient(std::move(dbClient)) {}

Json::Value ModelService::list(const std::string& search, int page, int pageSize) {
  const int offset = (page - 1) * pageSize;
  const std::string columns =
      "id::text AS id, name, type, version, batch_size, batch_mode, input_tensor_name, input_tensor_shape, precision, "
      "opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, status, built_at, trt_version, ever_built ";
  const std::string match = Searcher::clause(
      {"id",
       "name",
       "type",
       "version",
       "batch_size",
       "batch_mode",
       "input_tensor_name",
       "input_tensor_shape",
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
    items.append(item(row));
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
      "SELECT id::text AS id, name, type, version, batch_size, batch_mode, input_tensor_name, input_tensor_shape, "
      "precision, opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, status, built_at, trt_version, ever_built "
      "FROM model WHERE id = $1::uuid",
      modelId);
  if (rows.empty()) {
    throw std::runtime_error("model not found");
  }
  Json::Value data = item(rows[0]);
  return data;
}

void ModelService::remove(const std::string& modelId) {
  const auto used = dbClient->execSqlSync(
      "SELECT id FROM pipeline_gie WHERE model_id = $1::uuid LIMIT 1", modelId);
  if (!used.empty()) {
    throw std::runtime_error(model::ERR_OCCUPIED);
  }
  const auto rows = dbClient->execSqlSync(
      "DELETE FROM model WHERE id = $1::uuid AND status <> $2 RETURNING source_path, engine_path",
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
  if (!accepted(name.asString(), type.asString(), batchSize.asInt(), batchMode.asString(), precision.asString())) {
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
      "id::text AS id, name, type, version, batch_size, batch_mode, input_tensor_name, input_tensor_shape, precision, "
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
                  "input_tensor_name = NULLIF($13, ''), input_tensor_shape = NULLIF($14, ''), "
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
      row[model::KEY_INPUT_TENSOR_NAME].isNull() ? "" : row[model::KEY_INPUT_TENSOR_NAME].asString(),
      row[model::KEY_INPUT_TENSOR_SHAPE].isNull() ? "" : row[model::KEY_INPUT_TENSOR_SHAPE].asString(),
      row[model::KEY_OPT_LEVEL].isNull() ? "" : std::to_string(row[model::KEY_OPT_LEVEL].asInt()),
      row[model::KEY_DEVICE].asInt(),
      row[model::KEY_WORKSPACE].asInt(),
      row[model::KEY_STATIC_PLUGIN].isNull() ? "" : row[model::KEY_STATIC_PLUGIN].asString());
  if (rows.empty()) {
    throw std::runtime_error("model not updated");
  }
  Json::Value data = item(rows[0]);
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
  data.exportOnnx = sections.isMember(model::SECTION_EXPORT_ONNX) ? yaml(sections[model::SECTION_EXPORT_ONNX]) : "";
  data.exportTrt = sections.isMember(model::SECTION_EXPORT_TRT) ? yaml(sections[model::SECTION_EXPORT_TRT]) : "";
  return data;
}

void ModelService::buildEnd(const std::string& modelId, const Json::Value& result) {
  if (result.isNull()) {
    dbClient->execSqlSync(
        "UPDATE model SET status = CASE WHEN ever_built THEN $2 ELSE $3 END WHERE id = $1::uuid",
        modelId,
        model::STATUS_BUILT,
        model::STATUS_NOT_BUILT);
  } else {
    const Json::Value& inputTensorName = result[model::KEY_INPUT_TENSOR_NAME];
    const Json::Value& inputTensorShape = result[model::KEY_INPUT_TENSOR_SHAPE];
    const Json::Value& optLevel = result[model::KEY_OPT_LEVEL];
    const Json::Value& staticPlugin = result[model::KEY_STATIC_PLUGIN];
    const Json::Value& maxDet = result[model::KEY_MAX_DET];
    const Json::Value& conf = result[model::KEY_CONF];
    const Json::Value& iou = result[model::KEY_IOU];
    const Json::Value& numClass = result[model::KEY_NUM_CLASS];
    const Json::Value& classes = result[model::KEY_CLASSES];
    const Json::Value& sourcePath = result[model::KEY_SOURCE_PATH];
    const Json::Value& enginePath = result[model::KEY_ENGINE_PATH];
    const Json::Value& builtAt = result[model::KEY_BUILT_AT];
    const Json::Value& trtVersion = result[model::KEY_TRT_VERSION];
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    dbClient->execSqlSync(
        "UPDATE model SET type = $2, batch_size = $3, batch_mode = $4, input_tensor_name = $5, "
        "input_tensor_shape = $6, precision = $7, opt_level = $8, device = $9, workspace = $10, "
        "static_plugin = $11, max_det = $12, conf = $13, iou = $14, num_class = $15, classes = $16::jsonb, "
        "source_path = $17, engine_path = $18, status = $19, built_at = $20::timestamptz, trt_version = $21, "
        "ever_built = TRUE WHERE id = $1::uuid",
        modelId,
        result[model::KEY_TYPE].asString(),
        result[model::KEY_BATCH_SIZE].asInt(),
        result[model::KEY_BATCH_MODE].asString(),
        inputTensorName.isNull() ? std::nullopt : std::optional<std::string>(inputTensorName.asString()),
        inputTensorShape.isNull() ? std::nullopt : std::optional<std::string>(Json::writeString(builder, inputTensorShape)),
        result[model::KEY_PRECISION].asString(),
        optLevel.isNull() ? std::nullopt : std::optional<int>(optLevel.asInt()),
        result[model::KEY_DEVICE].asInt(),
        result[model::KEY_WORKSPACE].asInt(),
        staticPlugin.isNull() ? std::nullopt : std::optional<std::string>(staticPlugin.asString()),
        maxDet.isNull() ? std::nullopt : std::optional<int>(maxDet.asInt()),
        conf.isNull() ? std::nullopt : std::optional<double>(conf.asDouble()),
        iou.isNull() ? std::nullopt : std::optional<double>(iou.asDouble()),
        numClass.isNull() ? std::nullopt : std::optional<int>(numClass.asInt()),
        classes.isNull() ? std::nullopt : std::optional<std::string>(Json::writeString(builder, classes)),
        sourcePath.isNull() ? std::nullopt : std::optional<std::string>(sourcePath.asString()),
        enginePath.isNull() ? std::nullopt : std::optional<std::string>(enginePath.asString()),
        result[model::KEY_STATUS].asString(),
        builtAt.isNull() ? std::nullopt : std::optional<std::string>(builtAt.asString()),
        trtVersion.isNull() ? std::nullopt : std::optional<std::string>(std::to_string(trtVersion.asInt())));
  }
}

bool ModelService::checkOnnxZip(const std::string& path) {
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

std::string ModelService::yaml(const Json::Value& fields) {
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
  const bool accepted = output && (!zip || checkOnnxZip(staged.string()));
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
      "id::text AS id, name, type, version, batch_size, batch_mode, input_tensor_name, input_tensor_shape, precision, "
      "opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, status, built_at, trt_version, ever_built";
  const auto rows = dbClient->execSqlSync(
      std::string("UPDATE model SET source_path = $2 WHERE id = $1::uuid RETURNING ") + columns,
      modelId,
      source.string());
  if (rows.empty()) {
    throw std::runtime_error("model not updated");
  }
  Json::Value data = item(rows[0]);
  return data;
}

void ModelService::importModel(const std::string& fileName, const std::string& fileContent) {
  const std::filesystem::path file(std::filesystem::path(fileName).filename());
  const std::string name = file.string();
  const bool plain = name.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._-") ==
      std::string::npos;
  const bool zip = file.extension() == model::SUFFIX_ZIP;
  if (!plain || !zip) {
    throw std::runtime_error(model::ERR_IMPORT);
  }
  std::filesystem::create_directories(model::UPLOAD_DIR);
  const std::filesystem::path staged = std::filesystem::path(model::UPLOAD_DIR) / name;
  std::ofstream output(staged, std::ios::binary);
  output.write(fileContent.data(), static_cast<std::streamsize>(fileContent.size()));
  output.close();
  const std::string command = "unzip -Z -1 '" + staged.string() + "'";
  FILE* pipe = popen(command.c_str(), "r");
  std::string listing;
  char buffer[256];
  while (pipe && std::fgets(buffer, sizeof(buffer), pipe)) {
    listing += buffer;
  }
  const int status = pipe ? pclose(pipe) : -1;
  std::filesystem::remove(staged);
  const bool listed = output && status != -1 && WIFEXITED(status) && WEXITSTATUS(status) == 0;
  bool labels = false;
  bool meta = false;
  int engines = 0;
  std::istringstream lines(listing);
  std::string entry;
  while (listed && std::getline(lines, entry)) {
    if (!entry.empty() && entry.back() == '\r') {
      entry.pop_back();
    }
    const bool directory = !entry.empty() && entry.back() == '/';
    const std::filesystem::path path(entry);
    labels = labels || path.filename() == model::FILE_LABELS;
    meta = meta || path.filename() == model::FILE_META;
    engines += !directory && path.extension() == model::SUFFIX_ENGINE ? 1 : 0;
  }
  const bool accepted = listed && labels && meta && engines == 1;
  if (!accepted) {
    throw std::runtime_error(model::ERR_IMPORT);
  }
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
  const Json::Value defaults = model::row();
  if (!accepted(name.asString(), type.asString(), batchSize.asInt(), batchMode.asString(), precision.asString())) {
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
      "id::text AS id, name, type, version, batch_size, batch_mode, input_tensor_name, input_tensor_shape, precision, "
      "opt_level, device, workspace, static_plugin, max_det, conf, iou, num_class, "
      "classes::text AS classes, source_path, status, built_at, trt_version, ever_built";
  const auto rows = dbClient->execSqlSync(
      std::string("INSERT INTO model (id, name, type, batch_size, batch_mode, precision, status, source_path, "
                  "max_det, conf, iou, input_tensor_name, input_tensor_shape, opt_level, device, workspace, "
                  "static_plugin, ever_built) "
                  "VALUES (gen_random_uuid(), $1, $2, $3, $4, $5, $6, NULLIF($7, ''), "
                  "NULLIF($8, '')::int, NULLIF($9, '')::numeric(4, 2), NULLIF($10, '')::numeric(4, 2), "
                  "NULLIF($11, ''), NULLIF($12, ''), NULLIF($13, '')::int, $14, $15, NULLIF($16, ''), $17) "
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
      data[model::KEY_INPUT_TENSOR_NAME].isNull() ? "" : data[model::KEY_INPUT_TENSOR_NAME].asString(),
      data[model::KEY_INPUT_TENSOR_SHAPE].isNull() ? "" : data[model::KEY_INPUT_TENSOR_SHAPE].asString(),
      data[model::KEY_OPT_LEVEL].isNull() ? "" : std::to_string(data[model::KEY_OPT_LEVEL].asInt()),
      data[model::KEY_DEVICE].asInt(),
      data[model::KEY_WORKSPACE].asInt(),
      data[model::KEY_STATIC_PLUGIN].isNull() ? "" : data[model::KEY_STATIC_PLUGIN].asString(),
      data[model::KEY_EVER_BUILT].asBool());
  if (rows.empty()) {
    throw std::runtime_error("model not created");
  }
  Json::Value result = item(rows[0]);
  return result;
}

bool ModelService::accepted(const std::string& name,
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

Json::Value ModelService::item(const drogon::orm::Row& row) {
  Json::Value classes;
  const bool parsed = !row["classes"].isNull() && Json::Reader().parse(row["classes"].as<std::string>(), classes);
  Json::Value node;
  node[model::KEY_ID] = row["id"].as<std::string>();
  node[model::KEY_NAME] = row["name"].as<std::string>();
  node[model::KEY_TYPE] = row["type"].as<std::string>();
  node[model::KEY_VERSION] = row["version"].isNull() ? Json::Value() : row["version"].as<std::string>();
  node[model::KEY_BATCH_SIZE] = row["batch_size"].as<int>();
  node[model::KEY_BATCH_MODE] = row["batch_mode"].as<std::string>();
  node[model::KEY_INPUT_TENSOR_NAME] =
      row["input_tensor_name"].isNull() ? Json::Value() : row["input_tensor_name"].as<std::string>();
  node[model::KEY_INPUT_TENSOR_SHAPE] =
      row["input_tensor_shape"].isNull() ? Json::Value() : row["input_tensor_shape"].as<std::string>();
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
