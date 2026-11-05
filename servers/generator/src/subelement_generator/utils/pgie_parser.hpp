#pragma once

#include <filesystem>
#include <set>
#include <string>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

class PgieParser {
public:
    PgieParser(
        std::filesystem::path modelZip,
        int runtimeBatchSize,
        YAML::Node classAttrs,
        int interval = 1);

    YAML::Node build();
    const nlohmann::json& inputShape() const;
    std::string inputName() const;
    std::set<int> classIds;
    nlohmann::json meta;

private:
    void getMetaFromZip();
    void initClass();

    std::filesystem::path modelZip;
    int runtimeBatchSize;
    YAML::Node classAttrs;
    int interval;
    int numClasses = 0;
    bool filterClasses = false;
};
