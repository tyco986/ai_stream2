#pragma once

#include <filesystem>
#include <string>

class KafkaGenerator {
public:
    KafkaGenerator();

    void write(const std::filesystem::path& savePath);
    std::string config;
};
