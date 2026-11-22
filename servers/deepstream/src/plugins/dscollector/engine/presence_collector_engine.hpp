#pragma once

#include <map>
#include <string>

#include "plugins/dscollector/engine/collector_engine.hpp"
#include "plugins/presenceeventcoder/engine/presence_event_coder.hpp"

class PresenceCollectorEngine : public CollectorEngine {
public:
    explicit PresenceCollectorEngine(int interval);

    void setDumpCodes(const char* codes);
    void setCapturerOutput(const char* path);
    const std::string& getDumpCodes() const;
    const std::string& getCapturerOutput() const;

protected:
    void appendExtra(std::ostringstream& json, NvDsFrameMeta* frame) override;

private:
    const PresenceEventMeta* eventMeta(NvDsFrameMeta* frame) const;
    unsigned slotCount(const PresenceEventMeta* event) const;
    bool codesHit(const PresenceEventMeta* event) const;
    unsigned int takeId(int padIndex);
    std::string absolutePath(const std::string& relative) const;
    void writePath(std::ostringstream& json, const char* key, const std::string& path) const;

    std::string dumpCodes;
    std::string capturerOutput;
    std::map<int, unsigned int> nextIds;
};
