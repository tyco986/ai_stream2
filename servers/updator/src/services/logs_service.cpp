#include "services/logs_service.hpp"

#include "constants/logger_const.hpp"
#include "logger/request_logger.hpp"

#include <filesystem>
#include <vector>

void Logs::remove() {
    RequestLogger::release();
    std::filesystem::create_directories(request_log::ROOT);
    std::vector<std::filesystem::path> files;
    for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(request_log::ROOT)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }
    for (const std::filesystem::path& path : files) {
        std::filesystem::remove(path);
    }
}
