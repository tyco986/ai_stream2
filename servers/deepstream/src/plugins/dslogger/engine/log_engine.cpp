#include <cmath>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <map>
#include <sstream>

#include <spdlog/sinks/rotating_file_sink.h>

#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/engine/log_engine.hpp"

LogEngine::LogEngine(int interval)
    : logInterval(interval),
      pipelineName(),
      counters(),
      loggers() {
    logInterval = logInterval < 0 ? 0 : logInterval;
}

LogEngine::~LogEngine() = default;

void LogEngine::setInterval(int interval) {
    logInterval = interval < 0 ? 0 : interval;
    counters.clear();
}

int LogEngine::interval() const {
    return logInterval;
}

bool LogEngine::named() const {
    return !pipelineName.empty();
}

void LogEngine::setPipelineName(const std::string& name) {
    pipelineName = pipelineName.empty() && !name.empty() ? name : pipelineName;
}

void LogEngine::appendExtra(std::ostringstream&, NvDsFrameMeta*) const {
}

bool LogEngine::shouldLog(int pad) {
    const int counter = counters[pad];
    const bool write = logInterval == 0 || counter % logInterval == 0;
    counters[pad] = write ? 1 : counter + 1;
    return write;
}

std::shared_ptr<spdlog::logger> LogEngine::loggerFor(int pad) {
    std::shared_ptr<spdlog::logger> logger = loggers[pad];
    if (!logger) {
        std::ostringstream folder;
        folder << std::setw(dslogger::PAD_WIDTH) << std::setfill('0') << pad;
        const std::filesystem::path directory = std::filesystem::path(dslogger::ROOT) / pipelineName / folder.str();
        std::filesystem::create_directories(directory);
        const std::filesystem::path path = directory / (std::string(page()) + dslogger::SUFFIX);
        const std::string name = std::string(page()) + "-" + folder.str();
        auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(path.string(), dslogger::MAX_BYTES, dslogger::MAX_FILES);
        logger = std::make_shared<spdlog::logger>(name, sink);
        logger->set_pattern("%v");
        logger->flush_on(spdlog::level::info);
        loggers[pad] = logger;
    }
    return logger;
}

std::string LogEngine::escapeLabel(const char* label) const {
    std::string out;
    const char* src = label != nullptr ? label : "";
    for (const char* cursor = src; *cursor != '\0'; ++cursor) {
        const char ch = *cursor;
        if (ch == '\\' || ch == '"') {
            out.push_back('\\');
        }
        if (ch != '\n' && ch != '\r') {
            out.push_back(ch);
        }
    }
    return out;
}

void LogEngine::appendDetFields(std::ostringstream& json, NvDsObjectMeta* object) const {
    const float left = object->rect_params.left;
    const float top = object->rect_params.top;
    const double logged = std::round(static_cast<double>(object->confidence) * dslogger::CONF_SCALE) / dslogger::CONF_SCALE;
    const int objectId = object->object_id == UNTRACKED_OBJECT_ID ? dslogger::MISSING_OBJECT_ID : static_cast<int>(object->object_id);
    json << static_cast<int>(std::lround(left)) << ","
         << static_cast<int>(std::lround(top)) << ","
         << static_cast<int>(std::lround(left + object->rect_params.width)) << ","
         << static_cast<int>(std::lround(top + object->rect_params.height)) << ","
         << std::fixed << std::setprecision(dslogger::CONF_DIGITS) << logged << ","
         << object->class_id << ",\"" << escapeLabel(object->obj_label) << "\","
         << objectId;
}

void LogEngine::appendObjectItem(std::ostringstream& json, NvDsObjectMeta* object) const {
    json << "[";
    appendDetFields(json, object);
    json << "]";
}

std::string LogEngine::buildLine(NvDsFrameMeta* frame, double latencyMs) const {
    std::ostringstream json;
    const std::time_t now = std::time(nullptr);
    std::tm local {};
    localtime_r(&now, &local);
    char timestamp[dslogger::TS_SIZE];
    std::strftime(timestamp, sizeof(timestamp), dslogger::TS_FORMAT, &local);
    json << "{\"ts\":\"" << timestamp << "\""
         << ",\"pad\":" << static_cast<int>(frame->pad_index)
         << ",\"source\":" << static_cast<int>(frame->source_id)
         << ",\"frame\":" << static_cast<int>(frame->frame_num)
         << ",\"latency\":";
    if (latencyMs == dslogger::LATENCY_NONE) {
        json << dslogger::JSON_NULL;
    } else {
        json << std::fixed << std::setprecision(dslogger::CONF_DIGITS) << latencyMs;
    }
    std::map<int, int> classCounts;
    int all = 0;
    for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
        auto* object = static_cast<NvDsObjectMeta*>(item->data);
        if (object != nullptr) {
            classCounts[object->class_id] += 1;
            all += 1;
        }
    }
    json << ",\"num\":{\"all\":" << all;
    for (const auto& entry : classCounts) {
        json << ",\"" << entry.first << "\":" << entry.second;
    }
    json << "}";
    appendExtra(json, frame);
    json << ",\"object\":[";
    bool first = true;
    for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
        auto* object = static_cast<NvDsObjectMeta*>(item->data);
        if (object != nullptr) {
            if (!first) {
                json << ",";
            }
            first = false;
            appendObjectItem(json, object);
        }
    }
    json << "]}";
    return json.str();
}

void LogEngine::processFrame(NvDsFrameMeta* frame, double latencyMs) {
    const int pad = static_cast<int>(frame->pad_index);
    const bool ready = !pipelineName.empty();
    if (ready && shouldLog(pad)) {
        loggerFor(pad)->info(buildLine(frame, latencyMs));
    }
}
