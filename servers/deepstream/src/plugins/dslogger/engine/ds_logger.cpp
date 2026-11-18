#include "plugins/dslogger/engine/ds_logger.hpp"

#include <chrono>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include <spdlog/sinks/daily_file_sink.h>

#include "constants/dslogger_const.hpp"
#include "plugins/dscollector/engine/collector_line.hpp"

DsLogger::DsLogger(int interval, std::string page)
    : logInterval(interval),
      logPage(std::move(page)),
      pipelineName(),
      counters(),
      loggers() {
    logInterval = logInterval < 1 ? 1 : logInterval;
}

void DsLogger::setInterval(int interval) {
    logInterval = interval < 1 ? 1 : interval;
    counters.clear();
}

void DsLogger::setPage(const std::string& page) {
    logPage = page;
    loggers.clear();
}

int DsLogger::interval() const {
    return logInterval;
}

const std::string& DsLogger::page() const {
    return logPage;
}

bool DsLogger::named() const {
    return !pipelineName.empty();
}

void DsLogger::setPipelineName(const std::string& name) {
    pipelineName = pipelineName.empty() && !name.empty() ? name : pipelineName;
}

bool DsLogger::shouldLog(int pad) {
    const int counter = counters[pad];
    const bool write = counter % logInterval == 0;
    counters[pad] = write ? 1 : counter + 1;
    return write;
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
    const bool ready = !pipelineName.empty() && !logPage.empty() && line != nullptr && shouldLog(pad);
    if (ready) {
        loggerFor(pad)->info(line);
    }
}
