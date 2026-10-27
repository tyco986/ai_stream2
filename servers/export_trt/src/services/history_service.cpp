#include "services/history_service.hpp"

#include "constants/exporter_const.hpp"
#include "constants/history_const.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

#include <zip.h>

void History::save(const std::string& fileName, const std::string& fileContent) {
    const std::filesystem::path root = history::ROOT;
    std::filesystem::create_directories(root);
    const std::filesystem::path path = root / std::filesystem::path(fileName).filename();
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(fileContent.data(), static_cast<std::streamsize>(fileContent.size()));
}

nlohmann::json History::meta(const std::string& fileName) {
    std::ifstream input(std::filesystem::path(history::ROOT) / fileName, std::ios::binary);
    const std::string fileContent{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    zip_error_t zipError;
    zip_error_init(&zipError);
    zip_source_t* source = zip_source_buffer_create(fileContent.data(), fileContent.size(), 0, &zipError);
    zip_t* zip = source ? zip_open_from_source(source, ZIP_RDONLY, &zipError) : nullptr;
    std::string metaBytes;
    const zip_int64_t count = zip ? zip_get_num_entries(zip, 0) : 0;
    for (zip_int64_t index = 0; index < count; ++index) {
        const std::string name = zip_get_name(zip, index, 0);
        const bool directoryEntry = !name.empty() && name.back() == '/';
        zip_stat_t stat;
        const int stated = directoryEntry ? -1 : zip_stat_index(zip, index, 0, &stat);
        const bool sized = stated == 0 && (stat.valid & ZIP_STAT_SIZE) != 0;
        zip_file_t* entry = sized ? zip_fopen_index(zip, index, 0) : nullptr;
        std::string bytes(sized ? static_cast<std::size_t>(stat.size) : 0, '\0');
        const zip_int64_t read = entry ? zip_fread(entry, bytes.data(), bytes.size()) : -1;
        if (entry) {
            zip_fclose(entry);
        }
        const bool metaFile = read == static_cast<zip_int64_t>(bytes.size()) && std::filesystem::path(name).filename() == exporter::META_JSON;
        metaBytes = metaFile ? bytes : metaBytes;
    }
    if (zip) {
        zip_close(zip);
    }
    zip_error_fini(&zipError);
    const nlohmann::json value = metaBytes.empty() ? nlohmann::json::object() : nlohmann::json::parse(metaBytes);
    return value;
}

nlohmann::json History::get(const std::string& fileName) {
    std::filesystem::create_directories(history::ROOT);
    std::vector<std::filesystem::path> paths;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(history::ROOT)) {
        const bool zip = entry.is_regular_file() && entry.path().extension() == exporter::ZIP_SUFFIX;
        const bool named = fileName.empty() || entry.path().filename() == fileName;
        if (zip && named) {
            paths.push_back(entry.path());
        }
    }
    std::sort(paths.begin(), paths.end());
    nlohmann::json data = nlohmann::json::object();
    for (const std::filesystem::path& path : paths) {
        data[path.filename().string()] = meta(path.filename().string());
    }
    return data;
}

std::string History::loadFileContent(const std::string& fileName) {
    std::ifstream input(std::filesystem::path(history::ROOT) / fileName, std::ios::binary);
    const std::string fileContent{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    return fileContent;
}

void History::remove(const std::string& fileName) {
    std::filesystem::remove(std::filesystem::path(history::ROOT) / fileName);
}
