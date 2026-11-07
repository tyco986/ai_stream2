#include "constants/dslogger_const.hpp"
#include "plugins/dslogger/engine/presence_log_engine.hpp"

PresenceLogEngine::PresenceLogEngine(int interval)
    : LogEngine(interval) {
}

const char* PresenceLogEngine::page() const {
    return dslogger::PAGE_PRESENCE;
}

const PresenceLogEngine::PresenceEventMeta* PresenceLogEngine::eventMeta(NvDsFrameMeta* frame) const {
    const PresenceEventMeta* event = nullptr;
    if (frame != nullptr) {
        for (NvDsMetaList* item = frame->frame_user_meta_list; item != nullptr && event == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == dslogger::PRESENCE_EVENT_USER_META) {
                event = static_cast<const PresenceEventMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return event;
}

bool PresenceLogEngine::active(NvDsFrameMeta* frame) const {
    const PresenceEventMeta* event = eventMeta(frame);
    bool found = false;
    if (event != nullptr) {
        unsigned count = event->numClasses;
        if (count > dslogger::PRESENCE_EVENT_CODE_LEN) {
            count = dslogger::PRESENCE_EVENT_CODE_LEN;
        }
        for (unsigned index = 0; index < count && !found; index++) {
            found = event->eventCodes[index] != '0';
        }
    }
    return found;
}

void PresenceLogEngine::appendExtra(std::ostringstream& json, NvDsFrameMeta* frame) const {
    const PresenceEventMeta* event = eventMeta(frame);
    json << ",\"eventcode\":";
    if (event == nullptr) {
        json << dslogger::JSON_NULL;
    } else {
        unsigned count = event->numClasses;
        if (count > dslogger::PRESENCE_EVENT_CODE_LEN) {
            count = dslogger::PRESENCE_EVENT_CODE_LEN;
        }
        json << "\"";
        for (unsigned index = 0; index < count; index++) {
            json << event->eventCodes[index];
        }
        json << "\"";
    }
}
