#pragma once

#include <nlohmann/json.hpp>
#include <string>

class FFmpegVideo {
public:
    nlohmann::json probe(const std::string& path) const;
};
