#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

class GeneratorManager {
public:
    using Factory = std::string (*)(const nlohmann::json& payload, const std::filesystem::path& configSaveDir);

    static std::vector<std::string> types();
    static YAML::Node schema(const std::string& type);
    static std::string generate(const std::string& type, const std::filesystem::path& configSaveDir, const nlohmann::json& payload);

private:
    static const std::map<std::string, Factory>& factories();
    static const std::map<std::string, YAML::Node>& schemas();
};
