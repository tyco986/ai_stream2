#pragma once

#include "logger/trt_logger.hpp"

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

struct ExportInput {
    std::string onnx;
    std::string labels;
    nlohmann::json meta;
};

struct ExportResult {
    nlohmann::json data;
    std::string fileName;
    std::string file;
};

class Exporter {
public:
    Exporter();
    ExportResult exportModel(const std::string& fileName, const std::string& fileContent, const nlohmann::json& config);

private:
    TrtLogger trtLogger;
    ExportInput exportStart(const std::string& fileContent);
    std::string exportEnd(const std::string& engineName, const std::string& engine, const std::string& labels, const std::string& meta);
    std::string exportBuild(const ExportInput& input, const nlohmann::json& result);
    std::string trtexecCommand(const nlohmann::json& result, const std::filesystem::path& source, const std::filesystem::path& engine);
};
