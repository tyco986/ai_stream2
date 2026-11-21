#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <spdlog/spdlog.h>

#include "nvdsmeta.h"

class DsLogger {
public:
    explicit DsLogger(std::string page);

    void setPage(const std::string& page);
    bool named() const;
    void setPipelineName(const std::string& name);
    void writeFrame(NvDsFrameMeta* frame);

private:
    std::string currentFileName() const;
    std::shared_ptr<spdlog::logger> loggerFor(int pad);

    std::string logPage;
    std::string pipelineName;
    std::unordered_map<int, std::shared_ptr<spdlog::logger>> loggers;
};
