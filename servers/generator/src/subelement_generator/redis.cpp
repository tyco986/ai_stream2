#include "subelement_generator/redis.hpp"

#include "constants/subelement_const.hpp"

#include <fstream>

RedisGenerator::RedisGenerator()
    : config(subelement::REDIS_CONFIG) {
}

void RedisGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    handle << config;
}
