#pragma once

#include <filesystem>
#include <string>

#include <nlohmann/json.hpp>

struct ExportResult {
    nlohmann::json data;
    std::filesystem::path archive;
};

class Exporter {
public:
    ExportResult exportModel(const std::string& fileName, const std::string& fileContent, const nlohmann::json& config);

private:
    std::filesystem::path exportStart(const std::string& fileName, const std::string& fileContent);
    void exportEnd(const std::filesystem::path& directory, const std::filesystem::path& archive);
    std::string trtexecCommand(const nlohmann::json& result, const std::filesystem::path& source, const std::filesystem::path& engine);
};
