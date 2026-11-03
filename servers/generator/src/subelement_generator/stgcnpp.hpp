#pragma once

#include <filesystem>
#include <string>

#include <yaml-cpp/yaml.h>

class StgcnppGenerator {
public:
    StgcnppGenerator(
        std::string modelEngineFile,
        std::string labelfilePath,
        std::string outputBlobNames,
        int batchSize = 1,
        int networkMode = 2,
        int gpuId = 0,
        int interval = 0,
        std::string tensorName = "input",
        int numPerson = 2,
        int clipLen = 100,
        int numJoints = 17);

    void updateConfig();
    void write(const std::filesystem::path& savePath);
    YAML::Node config;
    std::string tensorName;
    int numPerson;
    int clipLen;
    int numJoints;

private:
    static int nvinferSkip(int userInterval);

    std::string modelEngineFile;
    std::string labelfilePath;
    std::string outputBlobNames;
    int batchSize;
    int networkMode;
    int gpuId;
    int interval;
};
