#pragma once

#include "subelement_generator/pipeline.hpp"

#include <optional>
#include <string>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

// Always-on presence coder and capturer params for event generators.
class BaseEventGenerator : public PipelineGenerator {
public:
    BaseEventGenerator(std::string pipelineName, nlohmann::json eventCoder, std::optional<nlohmann::json> capturer = std::nullopt);

    std::string capturerCodes() const;
    std::string captureOutputDir() const;
    void appendEventCoder(const std::string& name = "nvpresencecoder");
    std::string afterAnalytics(const std::string& teeName = "tee_msg") const;
    void linkEventCoder(YAML::Node edges, const std::string& teeName = "tee_msg") const;

    std::string pipelineName;
    nlohmann::json eventCoder;
    nlohmann::json capturer;
};
