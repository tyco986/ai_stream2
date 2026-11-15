#pragma once

#include <atomic>
#include <string>

#include <nlohmann/json.hpp>
#include <yaml-cpp/yaml.h>

class History {
public:
    void save(const std::string& fileName, const std::string& fileContent);
    static void setEnabled(bool value);
    nlohmann::json get(const std::string& fileName);
    std::string loadFileContent(const std::string& fileName);
    void remove(const std::string& fileName);
    static nlohmann::json yaml2json(const YAML::Node& node);

private:
    static std::atomic<bool> enabled;
    nlohmann::json params(const std::string& fileName);
};
