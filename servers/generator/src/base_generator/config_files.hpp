#pragma once

#include <map>
#include <string>
#include <variant>

#include <yaml-cpp/yaml.h>

inline std::string configFileText(const std::variant<YAML::Node, std::string>& body) {
    const bool plain = std::holds_alternative<std::string>(body);
    std::string text = plain ? std::get<std::string>(body) : YAML::Dump(std::get<YAML::Node>(body));
    return text;
}

template <typename ConfigFile>
std::map<std::string, std::string> renderConfigFiles(const std::map<std::string, ConfigFile>& files, const YAML::Node& pipelineDocument, const std::string& pipelineConfigName) {
    std::map<std::string, std::string> rendered;
    for (const auto& item : files) {
        const ConfigFile body = item.first == pipelineConfigName ? ConfigFile(pipelineDocument) : item.second;
        rendered.emplace(item.first, configFileText(body));
    }
    return rendered;
}
