#pragma once

#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/engine/log_engine.hpp"
#include "plugins/presenceeventcoder/engine/presence_event_coder.hpp"

class PresenceLogEngine : public LogEngine {
public:
    explicit PresenceLogEngine(int interval);
    bool active(NvDsFrameMeta* frame) const;

protected:
    const char* page() const override;
    void appendExtra(std::ostringstream& json, NvDsFrameMeta* frame) const override;

private:
    const PresenceEventMeta* eventMeta(NvDsFrameMeta* frame) const;
    unsigned slotCount(const PresenceEventMeta* event) const;
};
