#pragma once

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

class SgieParser {
public:
    SgieParser(
        std::filesystem::path modelZip,
        int interval = 1);

    YAML::Node build();
    const nlohmann::json& metaJson() const;

private:
    void getMetaFromZip();

    std::filesystem::path modelZip;
    int interval;
    nlohmann::json meta;
};
