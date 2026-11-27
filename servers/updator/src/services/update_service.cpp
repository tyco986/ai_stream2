#include "services/update_service.hpp"

#include "constants/pack_const.hpp"

#include <fstream>
#include <stdexcept>
#include <vector>

#include <zip.h>

std::vector<UpdateImage> Update::load(const std::string& filePath) {
    extract(filePath);
    std::vector<UpdateImage> images;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(workRoot)) {
        if (entry.is_directory()) {
            images.push_back(parse(entry.path()));
        }
    }
    return images;
}

void Update::cleanup() {
    std::filesystem::remove_all(workRoot);
}

void Update::extract(const std::string& filePath) {
    workRoot = std::filesystem::temp_directory_path() / pack::UPDATE_WORK_DIRECTORY;
    std::filesystem::remove_all(workRoot);
    std::filesystem::create_directories(workRoot);
    int error = 0;
    zip_t* zip = zip_open(filePath.c_str(), ZIP_RDONLY, &error);
    if (!zip) {
        throw std::runtime_error("zip open failed");
    }
    const zip_int64_t count = zip_get_num_entries(zip, 0);
    for (zip_int64_t index = 0; index < count; ++index) {
        const char* name = zip_get_name(zip, index, 0);
        const std::string relative = name ? name : "";
        const bool folder = relative.empty() || relative.back() == '/';
        if (!folder) {
            const std::filesystem::path out = workRoot / relative;
            std::filesystem::create_directories(out.parent_path());
            zip_file_t* entry = zip_fopen_index(zip, index, 0);
            std::ofstream output(out, std::ios::binary | std::ios::trunc);
            std::vector<char> buffer(65536);
            zip_int64_t read = 1;
            while (entry && read > 0) {
                read = zip_fread(entry, buffer.data(), buffer.size());
                if (read > 0) {
                    output.write(buffer.data(), static_cast<std::streamsize>(read));
                }
            }
            if (entry) {
                zip_fclose(entry);
            }
        }
    }
    zip_close(zip);
}

UpdateImage Update::parse(const std::filesystem::path& directory) {
    std::ifstream indexFile(directory / "index.json");
    const nlohmann::json index = nlohmann::json::parse(indexFile);
    const nlohmann::json descriptor = index["manifests"].at(0);
    const nlohmann::json annotations = descriptor.value("annotations", nlohmann::json::object());
    const std::string ref = annotations.value(pack::ANNOTATION_REF, "");
    const std::size_t colon = ref.rfind(':');
    UpdateImage image;
    image.name = colon == std::string::npos ? ref : ref.substr(0, colon);
    image.tag = colon == std::string::npos ? "latest" : ref.substr(colon + 1);
    image.fromDigest = annotations.value(pack::ANNOTATION_FROM, "");
    image.toDigest = descriptor.at("digest").get<std::string>();
    const std::filesystem::path blobs = directory / "blobs" / "sha256";
    if (std::filesystem::exists(blobs)) {
        for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(blobs)) {
            if (entry.is_regular_file()) {
                const std::string digest = "sha256:" + entry.path().filename().string();
                std::ifstream input(entry.path());
                const nlohmann::json document = nlohmann::json::parse(input, nullptr, false);
                const bool manifest = document.is_object() && (document.contains("layers") || document.contains("manifests") || document.contains("config"));
                UpdateBlob blob;
                blob.kind = manifest ? "manifests" : "blobs";
                blob.digest = digest;
                blob.file = entry.path();
                blob.mediaType = manifest ? document.value("mediaType", pack::DEFAULT_MEDIA_TYPE) : "application/octet-stream";
                image.blobs.push_back(blob);
                if (digest == image.toDigest) {
                    image.toManifest = document;
                }
            }
        }
    }
    return image;
}
