#pragma once

#include <filesystem>
#include <string>

class RedisGenerator {
public:
    RedisGenerator();

    void write(const std::filesystem::path& savePath);
    std::string config;
};
