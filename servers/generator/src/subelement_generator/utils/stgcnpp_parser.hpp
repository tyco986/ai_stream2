#pragma once

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

class StgcnppParser {
public:
    StgcnppParser(
        std::filesystem::path modelZip,
        std::string pipelineName,
        int interval = 1);

    YAML::Node build();
    const nlohmann::json& metaJson() const;

private:
    void getMetaFromZip();

    std::filesystem::path modelZip;
    std::string pipelineName;
    int interval;
    nlohmann::json meta;
};
