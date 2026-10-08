#pragma once

#include <string>

#include <nlohmann/json.hpp>

class FFmpegRTSP {
public:
    nlohmann::json probe(const std::string& url) const;
};
