#pragma once

#include <sstream>
#include <string>

#include <gst/gst.h>

#include "nvds_latency_meta.h"
#include "nvdsmeta.h"

class CollectorEngine {
public:
    explicit CollectorEngine(int interval);
    virtual ~CollectorEngine() = default;

    CollectorEngine(const CollectorEngine&) = delete;
    CollectorEngine& operator=(const CollectorEngine&) = delete;

    void setInterval(int interval);
    int getInterval() const;
    void collect(NvDsBatchMeta* batch, NvDsFrameLatencyInfo* info, guint count);

protected:
    virtual void appendExtra(std::ostringstream& json, NvDsFrameMeta* frame);
    virtual void appendObjectItem(std::ostringstream& json, NvDsObjectMeta* object) const;
    void appendDetFields(std::ostringstream& json, NvDsObjectMeta* object) const;
    std::string escapeLabel(const char* label) const;

private:
    std::string buildLine(NvDsFrameMeta* frame, double latencyMs);
    double latencyFor(NvDsFrameLatencyInfo* info, guint count, NvDsFrameMeta* frame) const;

    int sampleInterval;
    int batchCounter;
};
