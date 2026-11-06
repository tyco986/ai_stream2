#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include "nvdsmeta.h"

class LogEngine {
public:
    explicit LogEngine(int interval);
    virtual ~LogEngine();

    LogEngine(const LogEngine&) = delete;
    LogEngine& operator=(const LogEngine&) = delete;

    void setInterval(int interval);
    int interval() const;

    void processFrame(NvDsFrameMeta* frame, double latencyMs);

protected:
    virtual const char* page() const = 0;
    virtual void appendExtra(std::ostringstream& json, NvDsFrameMeta* frame) const;
    virtual void appendObjectItem(std::ostringstream& json, NvDsObjectMeta* object) const;
    void appendDetFields(std::ostringstream& json, NvDsObjectMeta* object) const;
    std::string escapeLabel(const char* label) const;

private:
    bool shouldLog(int pad);
    std::shared_ptr<spdlog::logger> loggerFor(int pad);
    std::string buildLine(NvDsFrameMeta* frame, double latencyMs) const;

    int logInterval;
    std::unordered_map<int, int> counters;
    std::unordered_map<int, std::shared_ptr<spdlog::logger>> loggers;
};
