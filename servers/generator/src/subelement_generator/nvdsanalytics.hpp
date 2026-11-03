#pragma once

#include <filesystem>
#include <optional>

#include <yaml-cpp/yaml.h>

class NvdsanalyticsGenerator {
public:
    explicit NvdsanalyticsGenerator(std::optional<YAML::Node> config = std::nullopt);

    void write(const std::filesystem::path& savePath);
    YAML::Node config;

private:
    static YAML::Node defaultConfig();
};
