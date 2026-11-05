#pragma once

#include "libs/httplib.h"
#include "pipeline_runner/pipeline_runner.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class PipelineService {
public:
    static bool status();
    static std::filesystem::path prepare(const httplib::FormData& config, const std::vector<httplib::FormData>& models);
    static void start(const std::filesystem::path& configDir);
    static void stop();

private:
    static std::optional<PipelineRunner> runner;

    static void extract(const std::string& bytes, const std::filesystem::path& dest);
};
