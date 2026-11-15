#pragma once

#include "plugins/dscollector/engine/collector_engine.hpp"
#include "plugins/presenceeventcoder/engine/presence_event_coder.hpp"

class PresenceCollectorEngine : public CollectorEngine {
public:
    PresenceCollectorEngine() = default;

protected:
    void appendExtra(std::ostringstream& json, NvDsFrameMeta* frame) const override;

private:
    const PresenceEventMeta* eventMeta(NvDsFrameMeta* frame) const;
    unsigned slotCount(const PresenceEventMeta* event) const;
};
