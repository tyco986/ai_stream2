#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include "nvdsmeta.h"

class DsLogger {
public:
    DsLogger(int interval, std::string page);

    void setInterval(int interval);
    void setPage(const std::string& page);
    int interval() const;
    const std::string& page() const;
    bool named() const;
    void setPipelineName(const std::string& name);
    void writeFrame(NvDsFrameMeta* frame);

private:
    bool shouldLog(int pad);
    std::string currentFileName() const;
    std::shared_ptr<spdlog::logger> loggerFor(int pad);

    int logInterval;
    std::string logPage;
    std::string pipelineName;
    std::unordered_map<int, int> counters;
    std::unordered_map<int, std::shared_ptr<spdlog::logger>> loggers;
};
