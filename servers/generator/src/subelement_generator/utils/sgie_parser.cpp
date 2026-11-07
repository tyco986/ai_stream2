#include "subelement_generator/utils/sgie_parser.hpp"

#include <stdexcept>
#include <utility>

#include <zip.h>

#include "constants/sgie_parser_const.hpp"

SgieParser::SgieParser(
    std::filesystem::path modelZip,
    int interval)
    : modelZip(std::move(modelZip)),
      interval(interval),
      meta(nullptr) {
    getMetaFromZip();
}

void SgieParser::getMetaFromZip() {
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
    const int stated = zip_stat(archive, sgie_parser::META_JSON_NAME, 0, &stat);
    zip_file_t* file = stated == 0 ? zip_fopen(archive, sgie_parser::META_JSON_NAME, 0) : nullptr;
    std::string buffer(file != nullptr ? stat.size : 0, '\0');
    const zip_int64_t readSize = file != nullptr ? zip_fread(file, buffer.data(), buffer.size()) : -1;
    if (file != nullptr) {
        zip_fclose(file);
    }
    zip_close(archive);
    if (stated != 0) {
        throw std::runtime_error(std::string(sgie_parser::META_JSON_NAME) + " not found in modelZip");
    }
    if (file == nullptr) {
        throw std::runtime_error("failed to read meta.json in modelZip");
    }
    if (readSize < 0 || static_cast<zip_uint64_t>(readSize) != buffer.size()) {
        throw std::runtime_error("short read of meta.json in modelZip");
    }
    meta = nlohmann::json::parse(buffer);
}

const nlohmann::json& SgieParser::metaJson() const {
    return meta;
}

YAML::Node SgieParser::build() {
    const std::string precision = meta.at("precision").get<std::string>();
    const auto precisionIt = sgie_parser::NETWORK_MODE_MAP.find(precision);
    if (precisionIt == sgie_parser::NETWORK_MODE_MAP.end()) {
        throw std::invalid_argument("unsupported precision in meta.json: " + precision);
    }
    const std::string type = meta.at("type").get<std::string>();
    if (!sgie_parser::SUPPORTED_TYPES.contains(type)) {
        throw std::invalid_argument("unsupported type in meta.json: " + type);
    }
    const std::string batchMode = meta.at("batch_mode").get<std::string>();
    const int engineBatchSize = meta.at("batch_size").get<int>();
    if (batchMode != "static" && batchMode != "dynamic") {
        throw std::invalid_argument("unsupported batch_mode in meta.json: " + batchMode);
    }
    if (engineBatchSize <= 0) {
        throw std::invalid_argument("batch_size in meta.json must be > 0");
    }
    const nlohmann::json& inputTensors = meta.at("input_tensors");
    if (!inputTensors.is_object() || inputTensors.size() != 1) {
        throw std::invalid_argument("expected one input tensor in meta.json");
    }
    const nlohmann::json& inputShape = inputTensors.begin().value();
    if (!inputShape.is_array() || inputShape.size() != 4) {
        throw std::invalid_argument("expected NCHW input tensor shape");
    }
    const std::string inferDims =
        std::to_string(inputShape.at(1).get<int>()) + ";" +
        std::to_string(inputShape.at(2).get<int>()) + ";" +
        std::to_string(inputShape.at(3).get<int>());
    std::string outputBlobNames;
    for (const auto& item : meta.at("output_tensors").items()) {
        if (!outputBlobNames.empty()) {
            outputBlobNames += ";";
        }
        outputBlobNames += item.key();
    }
    const std::filesystem::path modelDir =
        std::filesystem::path(sgie_parser::MODELS_DIR) / modelZip.stem();
    YAML::Node parsed(YAML::NodeType::Map);
    parsed["property"]["model-engine-file"] = (modelDir / meta.at("engine_name").get<std::string>()).string();
    parsed["property"]["labelfile-path"] = (modelDir / sgie_parser::LABELS_NAME).string();
    parsed["property"]["batch-size"] = engineBatchSize;
    parsed["property"]["network-mode"] = precisionIt->second;
    parsed["property"]["gpu-id"] = meta.at("device").get<int>();
    parsed["property"]["interval"] = interval;
    parsed["property"]["infer-dims"] = inferDims;
    parsed["property"]["output-blob-names"] = outputBlobNames;
    return parsed;
}
