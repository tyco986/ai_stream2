#pragma once

#include "subelement_generator/pipeline.hpp"

#include <filesystem>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

// Always-on presence coder and capturer params for event generators.
class BaseEventGenerator : public PipelineGenerator {
public:
    BaseEventGenerator(std::string pipelineName, std::filesystem::path capturerOutput, nlohmann::json eventCoder, std::optional<nlohmann::json> capturer = std::nullopt);

    std::string dumpCodes(const nlohmann::json& collector) const;
    std::string labelTask() const;
    void appendEventCoder(int interval, const nlohmann::json& collector, const std::string& name = "presenceeventcoder");
    std::string afterAnalytics(const std::string& teeName = "tee_msg") const;
    void linkEventCoder(YAML::Node edges, const std::string& teeName = "tee_msg") const;

    std::string pipelineName;
    std::filesystem::path capturerOutput;
    nlohmann::json eventCoder;
    nlohmann::json capturer;
};
