#pragma once

#include <filesystem>

#include <yaml-cpp/yaml.h>

class NvtrackerGenerator {
public:
    NvtrackerGenerator(int maxShadowTrackingAge = 30, int earlyTerminationAge = 1, int probationAge = 3);

    void write(const std::filesystem::path& savePath);
    YAML::Node config;

private:
    int maxShadowTrackingAge;
    int earlyTerminationAge;
    int probationAge;
};
