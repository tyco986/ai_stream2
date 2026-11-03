#include "subelement_generator/kafka.hpp"

#include "constants/subelement_const.hpp"

#include <fstream>

KafkaGenerator::KafkaGenerator()
    : config(subelement::KAFKA_CONFIG) {
}

void KafkaGenerator::write(const std::filesystem::path& savePath) {
    const std::filesystem::path path = std::filesystem::absolute(savePath);
    std::filesystem::create_directories(path.parent_path());
    std::ofstream handle;
    handle.exceptions(std::ios::failbit | std::ios::badbit);
    handle.open(path);
    handle << config;
}
