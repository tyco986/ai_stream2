#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

class NvmsgconvGenerator {
public:
    explicit NvmsgconvGenerator(std::vector<std::string> streamNames);

    void write(const std::filesystem::path& savePath);
    YAML::Node config;

private:
    std::vector<std::string> streamNames;
};
