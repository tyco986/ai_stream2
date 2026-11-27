#include "services/pack_service.hpp"

#include "constants/pack_const.hpp"

#include <fstream>
#include <set>
#include <stdexcept>

#include <zip.h>

std::filesystem::path Pack::createDirectoryInTmp(const std::vector<std::string>& names) {
    workRoot = std::filesystem::temp_directory_path() / pack::WORK_DIRECTORY;
    std::filesystem::remove_all(workRoot);
    std::filesystem::create_directories(workRoot);
    for (const std::string& name : names) {
        std::filesystem::create_directories(workRoot / folder(name) / "blobs" / "sha256");
    }
    return workRoot;
}

std::filesystem::path Pack::blobFile(const std::string& name, const std::string& digest) {
    const std::string hex = digest.substr(digest.find(':') + 1);
    return workRoot / folder(name) / "blobs" / "sha256" / hex;
}

std::vector<PackBlob> Pack::missing(const std::vector<nlohmann::json>& fromManifests, const std::vector<nlohmann::json>& toManifests) {
    std::set<std::string> present;
    for (const nlohmann::json& document : fromManifests) {
        if (document.contains("config")) {
            present.insert(document["config"]["digest"].get<std::string>());
        }
        if (document.contains("layers")) {
            for (const nlohmann::json& layer : document["layers"]) {
                present.insert(layer["digest"].get<std::string>());
            }
        }
        if (document.contains("manifests")) {
            for (const nlohmann::json& child : document["manifests"]) {
                present.insert(child["digest"].get<std::string>());
            }
        }
    }
    std::vector<PackBlob> result;
    std::set<std::string> seen;
    for (const nlohmann::json& document : toManifests) {
        std::vector<PackBlob> items;
        if (document.contains("config")) {
            items.push_back({"blobs", document["config"]["digest"].get<std::string>()});
        }
        if (document.contains("layers")) {
            for (const nlohmann::json& layer : document["layers"]) {
                items.push_back({"blobs", layer["digest"].get<std::string>()});
            }
        }
        if (document.contains("manifests")) {
            for (const nlohmann::json& child : document["manifests"]) {
                items.push_back({"manifests", child["digest"].get<std::string>()});
            }
        }
        for (const PackBlob& item : items) {
            const bool skip = present.contains(item.digest) || seen.contains(item.digest);
            if (!skip) {
                seen.insert(item.digest);
                result.push_back(item);
            }
        }
    }
    return result;
}

void Pack::layout(const PackImage& image) {
    const std::filesystem::path directory = workRoot / folder(image.name);
    const std::filesystem::path manifest = blobFile(image.name, image.toDigest);
    nlohmann::json annotations = {{pack::ANNOTATION_REF, image.name + ":" + image.tag}};
    if (!image.fromDigest.empty()) {
        annotations[pack::ANNOTATION_FROM] = image.fromDigest;
    }
    const nlohmann::json index = {
        {"schemaVersion", 2},
        {"manifests", nlohmann::json::array({
            {
                {"mediaType", image.toManifest.value("mediaType", pack::DEFAULT_MEDIA_TYPE)},
                {"digest", image.toDigest},
                {"size", std::filesystem::file_size(manifest)},
                {"annotations", annotations},
            },
        })},
    };
    std::ofstream layoutFile(directory / "oci-layout");
    layoutFile << pack::OCI_LAYOUT;
    std::ofstream indexFile(directory / "index.json");
    indexFile << index.dump();
}

std::string Pack::archive() {
    const std::filesystem::path zipPath = std::filesystem::path(pack::OUTPUT_DIRECTORY) / pack::FILE_NAME;
    std::filesystem::create_directories(zipPath.parent_path());
    int error = 0;
    zip_t* zip = zip_open(zipPath.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!zip) {
        throw std::runtime_error("zip open failed");
    }
    for (const std::filesystem::directory_entry& image : std::filesystem::directory_iterator(workRoot)) {
        const bool imageDir = image.is_directory();
        if (imageDir) {
            const std::filesystem::path directory = image.path();
            const std::string prefix = directory.filename().string();
            for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(directory)) {
                const bool folder = entry.is_directory();
                const std::string relative = prefix + "/" + std::filesystem::relative(entry.path(), directory).generic_string();
                if (folder) {
                    zip_dir_add(zip, relative.c_str(), ZIP_FL_ENC_UTF_8);
                }
                if (!folder) {
                    zip_source_t* source = zip_source_file(zip, entry.path().c_str(), 0, 0);
                    const zip_int64_t index = zip_file_add(zip, relative.c_str(), source, ZIP_FL_ENC_UTF_8);
                    zip_set_file_compression(zip, static_cast<zip_uint64_t>(index), ZIP_CM_STORE, 0);
                }
            }
        }
    }
    zip_close(zip);
    std::filesystem::remove_all(workRoot);
    return zipPath.string();
}

std::string Pack::folder(const std::string& name) {
    std::string folder = name;
    for (char& character : folder) {
        character = character == '/' ? '_' : character;
    }
    return folder;
}
