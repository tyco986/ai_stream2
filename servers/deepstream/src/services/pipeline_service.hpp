#pragma once

#include "libs/httplib.h"
#include "pipeline_runner/pipeline_runner.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

struct zip;
typedef struct zip zip_t;
struct zip_error;
typedef struct zip_error zip_error_t;

class PipelineService {
public:
    static PipelineStatus status();
    static std::filesystem::path prepare(const httplib::FormData& config,
                                         const std::vector<httplib::FormData>& models,
                                         const httplib::FormData& media);
    static std::filesystem::path prepareLocal(const std::string& configPath);
    static void start(const std::filesystem::path& configDir);
    static void stop();

private:
    static std::optional<PipelineRunner> runner;

    static void extract(const std::string& bytes, const std::filesystem::path& dest);
    static void extractPath(const std::string& path, const std::filesystem::path& dest);
    static void writeEntries(zip_t* archive, const std::filesystem::path& dest, zip_error_t& error);
    static void collectModelZips(const YAML::Node& node, std::vector<std::string>& paths);
    static void extractMedia(const std::string& bytes);
};
