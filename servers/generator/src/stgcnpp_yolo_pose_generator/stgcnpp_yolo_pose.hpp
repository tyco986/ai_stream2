#pragma once

#include "subelement_generator/stgcnpp.hpp"
#include "subelement_generator/stgcnpp_preprocess.hpp"
#include "subelement_generator/utils/stgcnpp_parser.hpp"

#include <filesystem>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

// ST-GCN++ classifier and skeleton preprocess shared by YOLO-pose generators.
class StgcnppYoloPose {
public:
    static constexpr const char* CONFIG_NAME = "sgie1.yml";
    static constexpr const char* META_NAME = "sgie1_meta.json";
    static constexpr const char* PREPROCESS_CONFIG_NAME = "preprocess_stgcnpp.txt";
    static constexpr const char* DRAWER_ELEMENT = "nvstgcnppfadedrawerwithtracker";

    StgcnppYoloPose(nlohmann::json stgcnpp, int streamCount);

    void init(int pgieInterval);
    YAML::Node params() const;
    void applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) const;
    static YAML::Node drawerProperties(const nlohmann::json& drawer);

    YAML::Node config;
    std::string preprocessIni;
    std::string metaText;
    int batchSize = 0;
    int gpuId = 0;
    int uniqueId = 0;

private:
    nlohmann::json stgcnpp;
    int streamCount;
    std::optional<StgcnppParser> parser;
    std::optional<StgcnppGenerator> generator;
    std::optional<StgcnppPreprocessGenerator> preprocessGenerator;
};
