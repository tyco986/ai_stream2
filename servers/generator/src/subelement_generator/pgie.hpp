#pragma once

#include <filesystem>
#include <optional>
#include <string>

#include <yaml-cpp/yaml.h>

class PgieGenerator {
public:
    PgieGenerator(
        std::string modelEngineFile,
        std::string labelfilePath,
        int batchSize = 1,
        int networkMode = 2,
        int gpuId = 0,
        int interval = 0,
        std::optional<YAML::Node> classAttrs = std::nullopt,
        std::optional<std::string> filterOutClassIds = std::nullopt,
        std::optional<int> numDetectedClasses = std::nullopt);

    void updateConfig();
    void write(const std::filesystem::path& savePath);
    YAML::Node config;

private:
    static int nvinferSkip(int userInterval);

    int gpuId;
    int interval;
    std::optional<YAML::Node> classAttrs;
    std::optional<std::string> filterOutClassIds;
    int batchSize;
    int networkMode;
    std::optional<int> numDetectedClasses;
    std::string modelEngineFile;
    std::string labelfilePath;
};
