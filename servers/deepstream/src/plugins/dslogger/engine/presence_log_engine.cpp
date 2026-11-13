#include <iomanip>
#include <string>

#include "constants/dslogger_const.hpp"
#include "constants/event_meta_const.hpp"
#include "plugins/dslogger/engine/presence_log_engine.hpp"

PresenceLogEngine::PresenceLogEngine(int interval)
    : LogEngine(interval) {
}

const char* PresenceLogEngine::page() const {
    return dslogger::PAGE_PRESENCE;
}

const PresenceEventMeta* PresenceLogEngine::eventMeta(NvDsFrameMeta* frame) const {
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

unsigned PresenceLogEngine::slotCount(const PresenceEventMeta* event) const {
    unsigned count = 0;
    if (event != nullptr) {
        count = event->event.slotCount;
        if (count > static_cast<unsigned>(eventmeta::SLOT_COUNT)) {
            count = static_cast<unsigned>(eventmeta::SLOT_COUNT);
        }
    }
    return count;
}

bool PresenceLogEngine::active(NvDsFrameMeta* frame) const {
    const PresenceEventMeta* event = eventMeta(frame);
    const unsigned count = slotCount(event);
    bool found = false;
    for (unsigned index = 0; index < count && !found; index++) {
        found = event->event.slots[index].code != eventmeta::ABSENT;
    }
    return found;
}

void PresenceLogEngine::appendExtra(std::ostringstream& json, NvDsFrameMeta* frame) const {
    const PresenceEventMeta* event = eventMeta(frame);
    json << ",\"event\":";
    if (event == nullptr) {
        json << dslogger::JSON_NULL;
    } else {
        const unsigned count = slotCount(event);
        std::string label;
        bool firstName = true;
        json << "{\"event_code\":\"";
        for (unsigned index = 0; index < count; index++) {
            const EventSlot& slot = event->event.slots[index];
            json << slot.code;
            if (slot.code == eventmeta::ALERT && slot.name[0] != '\0') {
                if (!firstName) {
                    label.push_back(',');
                }
                firstName = false;
                label += slot.name;
            }
        }
        json << "\",\"event_label\":\"" << escapeLabel(label.c_str())
             << "\",\"debouncer\":{\"max_length\":" << event->debouncer.length
             << ",\"ratio_threshold\":" << std::fixed << std::setprecision(dslogger::CONF_DIGITS) << event->debouncer.threshold
             << ",\"current_length\":[";
        for (unsigned index = 0; index < count; index++) {
            if (index > 0) {
                json << ",";
            }
            json << event->debouncer.slots[index].filled;
        }
        json << "],\"counts\":[";
        for (unsigned index = 0; index < count; index++) {
            if (index > 0) {
                json << ",";
            }
            const PresenceDebouncerSlot& slot = event->debouncer.slots[index];
            json << "[";
            for (int code = 0; code < presenceeventcoder::CODE_BOUNDS; code++) {
                if (code > 0) {
                    json << ",";
                }
                json << slot.counts[code];
            }
            json << "]";
        }
        json << "],\"ratio\":[";
        for (unsigned index = 0; index < count; index++) {
            if (index > 0) {
                json << ",";
            }
            const PresenceDebouncerSlot& slot = event->debouncer.slots[index];
            if (slot.filled == event->debouncer.length) {
                json << std::fixed << std::setprecision(dslogger::CONF_DIGITS) << slot.ratio;
            } else {
                json << dslogger::JSON_NULL;
            }
        }
        json << "]}}";
    }
}
