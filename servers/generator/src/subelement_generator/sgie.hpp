#pragma once

#include <filesystem>
#include <string>

#include <yaml-cpp/yaml.h>

class SgieGenerator {
public:
    SgieGenerator(
        std::string modelEngineFile,
        std::string labelfilePath,
        std::string inferDims,
        std::string outputBlobNames,
        int batchSize = 1,
        int networkMode = 2,
        int gpuId = 0,
        int interval = 0);

    void updateConfig();
    void write(const std::filesystem::path& savePath);
    YAML::Node config;

private:
    static int nvinferSkip(int userInterval);

    std::string modelEngineFile;
    std::string labelfilePath;
    std::string inferDims;
    std::string outputBlobNames;
    int batchSize;
    int networkMode;
    int gpuId;
    int interval;
};
