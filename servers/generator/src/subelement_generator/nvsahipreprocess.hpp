#pragma once

#include "constants/subelement_const.hpp"

#include <filesystem>
#include <string>
#include <vector>

class NvsahipreprocessGenerator {
public:
    NvsahipreprocessGenerator(
        std::string networkInputShape = subelement::SAHI_NETWORK_INPUT_SHAPE,
        int targetUniqueIds = 1,
        int tensorDataType = 0,
        std::string tensorName = subelement::SAHI_TENSOR_NAME);

    void write(const std::filesystem::path& savePath);
    std::vector<std::string> config;

private:
    std::string networkInputShape;
    int targetUniqueIds;
    int tensorDataType;
    std::string tensorName;
};
