#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

struct PackImage {
    std::string name;
    std::string tag;
    std::string fromDigest;
    std::string toDigest;
    nlohmann::json toManifest;
};

struct PackBlob {
    std::string kind;
    std::string digest;
};

class Pack {
public:
    std::filesystem::path createDirectoryInTmp(const std::vector<std::string>& names);
    std::filesystem::path blobFile(const std::string& name, const std::string& digest);
    std::vector<PackBlob> missing(const std::vector<nlohmann::json>& fromManifests, const std::vector<nlohmann::json>& toManifests);
    void layout(const PackImage& image);
    std::string archive();

private:
    std::filesystem::path workRoot;
    std::string folder(const std::string& name);
};
