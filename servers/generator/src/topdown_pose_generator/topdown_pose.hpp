#pragma once

#include "subelement_generator/rtmpose_preprocess.hpp"
#include "subelement_generator/sgie.hpp"
#include "subelement_generator/utils/sgie_parser.hpp"

#include <filesystem>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

// SGIE and RTMPose preprocess configs shared by topdown-pose generators.
class TopdownPose {
public:
    static constexpr const char* SGIE_CONFIG_NAME = "sgie0.yml";
    static constexpr const char* RTMPOSE_PREPROCESS_CONFIG_NAME = "nvdspreprocess_rtmpose.txt";

    TopdownPose(nlohmann::json sgie, int streamCount);

    void init();
    void applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) const;
    YAML::Node sgieParams() const;

    YAML::Node sgieConfig;
    std::string preprocessIni;
    int batchSize = 0;
    int gpuId = 0;
    int uniqueId = 0;

private:
    nlohmann::json sgie;
    int streamCount;
    std::optional<SgieParser> sgieConfigParser;
    std::optional<SgieGenerator> sgieGenerator;
    std::optional<NvdspreprocessRtmposeGenerator> preprocessGenerator;
};
