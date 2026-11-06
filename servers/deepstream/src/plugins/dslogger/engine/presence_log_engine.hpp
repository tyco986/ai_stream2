#pragma once

#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/engine/log_engine.hpp"

class PresenceLogEngine : public LogEngine {
public:
    explicit PresenceLogEngine(int interval);

protected:
    const char* page() const override;
    void appendExtra(std::ostringstream& json, NvDsFrameMeta* frame) const override;

private:
    struct PresenceEventMeta {
        char eventCodes[dslogger::PRESENCE_EVENT_CODE_LEN + 1];
        char eventNames[dslogger::PRESENCE_EVENT_CODE_LEN][dslogger::PRESENCE_EVENT_NAME_LEN];
        unsigned int numClasses;
    };

    const PresenceEventMeta* eventMeta(NvDsFrameMeta* frame) const;
};
