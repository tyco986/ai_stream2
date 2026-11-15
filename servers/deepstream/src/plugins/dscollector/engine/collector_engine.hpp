#pragma once

#include <sstream>
#include <string>

#include "nvdsmeta.h"

class CollectorEngine {
public:
    CollectorEngine() = default;
    virtual ~CollectorEngine() = default;

    CollectorEngine(const CollectorEngine&) = delete;
    CollectorEngine& operator=(const CollectorEngine&) = delete;

    void attach(NvDsBatchMeta* batch, NvDsFrameMeta* frame, double latencyMs);
    std::string buildLine(NvDsFrameMeta* frame, double latencyMs) const;

protected:
    virtual void appendExtra(std::ostringstream& json, NvDsFrameMeta* frame) const;
    virtual void appendObjectItem(std::ostringstream& json, NvDsObjectMeta* object) const;
    void appendDetFields(std::ostringstream& json, NvDsObjectMeta* object) const;
    std::string escapeLabel(const char* label) const;
};
