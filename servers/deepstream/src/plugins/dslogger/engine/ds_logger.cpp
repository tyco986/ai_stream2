#include "plugins/dslogger/engine/ds_logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include <spdlog/sinks/daily_file_sink.h>

#include "constants/dslogger_const.hpp"
#include "plugins/dscollector/engine/collector_line.hpp"

DsLogger::DsLogger(std::string page)
    : logPage(std::move(page)),
      pipelineName(),
      loggers() {
}

void DsLogger::setPage(const std::string& page) {
    logPage = page;
    loggers.clear();
}

bool DsLogger::named() const {
    return !pipelineName.empty();
}

void DsLogger::setPipelineName(const std::string& name) {
    pipelineName = pipelineName.empty() && !name.empty() ? name : pipelineName;
}

std::string DsLogger::currentFileName() const {
    const std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local{};
    localtime_r(&now, &local);
    char date[dslogger::DATE_SIZE];
    std::strftime(date, sizeof(date), dslogger::DATE_FORMAT, &local);
    return logPage + "_" + date + dslogger::SUFFIX;
}

std::shared_ptr<spdlog::logger> DsLogger::loggerFor(int pad) {
    std::ostringstream folder;
    folder << std::setw(dslogger::PAD_WIDTH) << std::setfill('0') << pad;
    const std::filesystem::path directory = std::filesystem::path(dslogger::ROOT) / pipelineName / dslogger::PAD_DIR / folder.str();
    const std::filesystem::path current = directory / currentFileName();
    std::shared_ptr<spdlog::logger> logger = loggers[pad];
    const bool missing = !logger || !std::filesystem::exists(current);
    if (missing) {
        std::filesystem::create_directories(directory);
        const std::filesystem::path path = directory / (logPage + dslogger::SUFFIX);
        const std::string name = logPage + "-" + folder.str();
        auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
            path.string(), dslogger::ROTATION_HOUR, dslogger::ROTATION_MINUTE, false, dslogger::MAX_DAYS);
        logger = std::make_shared<spdlog::logger>(name, sink);
        logger->set_pattern("%v");
        logger->flush_on(spdlog::level::info);
        loggers[pad] = logger;
    }
    return logger;
}

void DsLogger::writeFrame(NvDsFrameMeta* frame) {
    const int pad = static_cast<int>(frame->pad_index);
    const char* line = collectorLine(frame);
    const bool ready = !pipelineName.empty() && !logPage.empty() && line != nullptr;
    if (ready) {
        loggerFor(pad)->info(line);
    }
}
