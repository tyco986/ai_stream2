#include "subelement_generator/utils/pgie_parser.hpp"

#include <stdexcept>
#include <utility>

#include <zip.h>

#include "constants/pgie_parser_const.hpp"

PgieParser::PgieParser(
    std::filesystem::path modelZip,
    int runtimeBatchSize,
    YAML::Node classAttrs,
    int interval)
    : modelZip(std::move(modelZip)),
      runtimeBatchSize(runtimeBatchSize),
      classAttrs(std::move(classAttrs)),
      interval(interval),
      meta(nullptr),
      numClasses(0),
      classIds(),
      filterClasses(false) {
    getMetaFromZip();
    initClass();
}

void PgieParser::getMetaFromZip() {
    if (!std::filesystem::is_regular_file(modelZip)) {
        throw std::runtime_error("modelZip not found: " + modelZip.string());
    }
    int error = 0;
    zip_t* archive = zip_open(modelZip.c_str(), ZIP_RDONLY, &error);
    if (archive == nullptr) {
        throw std::runtime_error("failed to open modelZip: " + modelZip.string());
    }
    zip_stat_t stat;
    zip_stat_init(&stat);
    const int stated = zip_stat(archive, pgie_parser::META_JSON_NAME, 0, &stat);
    zip_file_t* file = stated == 0 ? zip_fopen(archive, pgie_parser::META_JSON_NAME, 0) : nullptr;
    std::string buffer(file != nullptr ? stat.size : 0, '\0');
    const zip_int64_t readSize = file != nullptr ? zip_fread(file, buffer.data(), buffer.size()) : -1;
    if (file != nullptr) {
        zip_fclose(file);
    }
    zip_close(archive);
    if (stated != 0) {
        throw std::runtime_error(std::string(pgie_parser::META_JSON_NAME) + " not found in modelZip");
    }
    if (file == nullptr) {
        throw std::runtime_error("failed to read meta.json in modelZip");
    }
    if (readSize < 0 || static_cast<zip_uint64_t>(readSize) != buffer.size()) {
        throw std::runtime_error("short read of meta.json in modelZip");
    }
    meta = nlohmann::json::parse(buffer);
}

void PgieParser::initClass() {
    numClasses = static_cast<int>(meta.at("classes").size());
    if (!classAttrs.IsMap() || classAttrs.size() == 0) {
        throw std::invalid_argument("class_attrs cannot be empty");
    }
    bool hasAll = false;
    for (const auto& item : classAttrs) {
        const std::string classKey = item.first.Scalar();
        if (classKey == "all") {
            hasAll = true;
            continue;
        }
        const int classId = std::stoi(classKey);
        if (!classIds.insert(classId).second) {
            throw std::invalid_argument("duplicate class id in class_attrs: " + classKey);
        }
    }
    if (!classIds.empty()) {
        if (*classIds.rbegin() > numClasses - 1) {
            throw std::invalid_argument("class_attrs max id must be <= " + std::to_string(numClasses - 1));
        }
        if (*classIds.begin() < 0) {
            throw std::invalid_argument("class_attrs ids must be >= 0");
        }
    }
    if (hasAll) {
        classIds.clear();
        for (int classId = 0; classId < numClasses; ++classId) {
            classIds.insert(classId);
        }
        filterClasses = false;
    } else {
        if (classIds.empty()) {
            throw std::invalid_argument("class_attrs must contain all or at least one class id");
        }
        filterClasses = true;
    }
}

const nlohmann::json& PgieParser::inputShape() const {
    const nlohmann::json& inputTensors = meta.at("input_tensors");
    const nlohmann::json& shape = inputTensors.begin().value();
    return shape;
}

std::string PgieParser::inputName() const {
    const std::string name = meta.at("input_tensors").begin().key();
    return name;
}

YAML::Node PgieParser::build() {
    const std::string precision = meta.at("precision").get<std::string>();
    const auto precisionIt = pgie_parser::NETWORK_MODE_MAP.find(precision);
    if (precisionIt == pgie_parser::NETWORK_MODE_MAP.end()) {
        throw std::invalid_argument("unsupported precision in meta.json: " + precision);
    }
    const std::string type = meta.at("type").get<std::string>();
    if (!pgie_parser::SUPPORTED_TYPES.contains(type)) {
        throw std::invalid_argument("unsupported type in meta.json: " + type);
    }
    const std::string batchMode = meta.at("batch_mode").get<std::string>();
    const int engineBatchSize = meta.at("batch_size").get<int>();
    if (batchMode == "static") {
        if (runtimeBatchSize != engineBatchSize) {
            throw std::invalid_argument(
                "runtime_batch_size " + std::to_string(runtimeBatchSize) +
                " must equal engine batch_size " + std::to_string(engineBatchSize) + " for static model");
        }
    } else if (batchMode == "dynamic") {
        if (runtimeBatchSize > engineBatchSize) {
            throw std::invalid_argument(
                "runtime_batch_size " + std::to_string(runtimeBatchSize) +
                " must be <= engine batch_size " + std::to_string(engineBatchSize) + " for dynamic model");
        }
    } else {
        throw std::invalid_argument("unsupported batch_mode in meta.json: " + batchMode);
    }
    for (const auto& item : classAttrs) {
        if (!item.second.IsMap()) {
            throw std::invalid_argument("class_attrs value must be a map");
        }
        for (const auto& attr : item.second) {
            const std::string key = attr.first.Scalar();
            if (!pgie_parser::CLASS_ATTR_KEY_MAP.contains(key)) {
                throw std::invalid_argument("unsupported class_attrs key: " + key);
            }
        }
    }
    YAML::Node mappedClassAttrs(YAML::NodeType::Map);
    for (const auto& item : classAttrs) {
        const std::string classKey = item.first.Scalar();
        const std::string section = classKey == "all" ? "class-attrs-all" : "class-attrs-" + classKey;
        YAML::Node mapped(YAML::NodeType::Map);
        for (const auto& attr : item.second) {
            const std::string key = attr.first.Scalar();
            const YAML::Node& value = attr.second;
            if (!value || value.IsNull()) {
                continue;
            }
            if (key.starts_with("detected_") && value.as<int>() < 0) {
                continue;
            }
            mapped[pgie_parser::CLASS_ATTR_KEY_MAP.at(key)] = value;
        }
        mappedClassAttrs[section] = mapped;
    }
    std::string filterOutClassIds;
    if (filterClasses) {
        for (int classId = 0; classId < numClasses; ++classId) {
            if (classIds.contains(classId)) {
                continue;
            }
            if (!filterOutClassIds.empty()) {
                filterOutClassIds += ";";
            }
            filterOutClassIds += std::to_string(classId);
        }
    }
    const std::filesystem::path modelDir =
        std::filesystem::path(pgie_parser::RUNTIME_ROOT) / modelZip.stem();
    YAML::Node parsed(YAML::NodeType::Map);
    parsed["property"]["model-engine-file"] = (modelDir / meta.at("engine_name").get<std::string>()).string();
    parsed["property"]["labelfile-path"] = (modelDir / pgie_parser::LABELS_NAME).string();
    parsed["property"]["batch-size"] = runtimeBatchSize;
    parsed["property"]["network-mode"] = precisionIt->second;
    parsed["property"]["gpu-id"] = meta.at("device").get<int>();
    parsed["property"]["interval"] = interval;
    parsed["property"]["num-detected-classes"] = numClasses;
    if (filterClasses) {
        parsed["property"]["filter-out-class-ids"] = filterOutClassIds;
    }
    for (const auto& section : mappedClassAttrs) {
        parsed[section.first.as<std::string>()] = section.second;
    }
    return parsed;
}
