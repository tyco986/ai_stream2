#include "services/logs_service.hpp"

#include "constants/logger_const.hpp"
#include "logger/request_logger.hpp"

#include <filesystem>

void Logs::remove() {
    RequestLogger::release();
    std::filesystem::remove_all(request_log::ROOT);
    std::filesystem::create_directories(request_log::ROOT);
}
