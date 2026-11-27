#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

struct UpdateBlob {
    std::string kind;
    std::string digest;
    std::filesystem::path file;
    std::string mediaType;
};

struct UpdateImage {
    std::string name;
    std::string tag;
    std::string fromDigest;
    std::string toDigest;
    nlohmann::json toManifest;
    std::vector<UpdateBlob> blobs;
};

class Update {
public:
    std::vector<UpdateImage> load(const std::string& filePath);
    void cleanup();

private:
    std::filesystem::path workRoot;
    void extract(const std::string& filePath);
    UpdateImage parse(const std::filesystem::path& directory);
};
