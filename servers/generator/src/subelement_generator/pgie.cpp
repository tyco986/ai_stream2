#include "subelement_generator/pgie.hpp"

#include <fstream>
#include <stdexcept>
#include <utility>

int PgieGenerator::nvinferSkip(int userInterval) {
    if (userInterval < 1) {
        throw std::invalid_argument("interval must be >= 1");
    }
    const int skip = userInterval - 1;
    return skip;
}

PgieGenerator::PgieGenerator(
    std::string modelEngineFile,
    std::string labelfilePath,
    int batchSize,
    int networkMode,
    int gpuId,
    int interval,
    std::optional<YAML::Node> classAttrs,
    std::optional<std::string> filterOutClassIds,
    std::optional<int> numDetectedClasses)
    : gpuId(gpuId),
      interval(interval),
      classAttrs(std::move(classAttrs)),
      filterOutClassIds(std::move(filterOutClassIds)),
      batchSize(batchSize),
      networkMode(networkMode),
      numDetectedClasses(numDetectedClasses),
      modelEngineFile(std::move(modelEngineFile)),
      labelfilePath(std::move(labelfilePath)) {
    config["property"] = YAML::Node(YAML::NodeType::Map);
}

void PgieGenerator::updateConfig() {
    config["property"]["batch-size"] = batchSize;
    config["property"]["network-mode"] = networkMode;
    config["property"]["num-detected-classes"] = numDetectedClasses.has_value() ? YAML::Node(*numDetectedClasses) : YAML::Node(YAML::NodeType::Null);
    config["property"]["gpu-id"] = gpuId;
    config["property"]["interval"] = nvinferSkip(interval);
    config["property"]["model-engine-file"] = modelEngineFile;
    config["property"]["labelfile-path"] = labelfilePath;
    if (filterOutClassIds.has_value()) {
        config["property"]["filter-out-class-ids"] = *filterOutClassIds;
    } else {
        config["property"].remove("filter-out-class-ids");
    }
    if (classAttrs.has_value()) {
        for (const auto& section : *classAttrs) {
            const std::string sectionName = section.first.as<std::string>();
            YAML::Node merged = config[sectionName] ? YAML::Clone(config[sectionName]) : YAML::Node(YAML::NodeType::Map);
            for (const auto& attr : section.second) {
                merged[attr.first.as<std::string>()] = attr.second;
            }
            config[sectionName] = merged;
        }
    }
}

void PgieGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    handle << YAML::Dump(config);
}
