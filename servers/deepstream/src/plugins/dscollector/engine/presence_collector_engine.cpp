#include "plugins/dscollector/engine/presence_collector_engine.hpp"

#include <iomanip>
#include <string>

#include "constants/collector_const.hpp"
#include "constants/event_meta_const.hpp"
#include "constants/presenceeventcoder_const.hpp"

PresenceCollectorEngine::PresenceCollectorEngine(int interval) : CollectorEngine(interval) {
}

const PresenceEventMeta* PresenceCollectorEngine::eventMeta(NvDsFrameMeta* frame) const {
    const PresenceEventMeta* event = nullptr;
    if (frame != nullptr) {
        for (NvDsMetaList* item = frame->frame_user_meta_list; item != nullptr && event == nullptr; item = item->next) {
            auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
            if (userMeta != nullptr && userMeta->base_meta.meta_type == presenceeventcoder::USER_META) {
                event = static_cast<const PresenceEventMeta*>(userMeta->user_meta_data);
            }
        }
    }
    return event;
}

unsigned PresenceCollectorEngine::slotCount(const PresenceEventMeta* event) const {
    unsigned count = 0;
    if (event != nullptr) {
        count = event->event.slotCount;
        if (count > static_cast<unsigned>(eventmeta::SLOT_COUNT)) {
            count = static_cast<unsigned>(eventmeta::SLOT_COUNT);
        }
    }
    return count;
}

void PresenceCollectorEngine::appendExtra(std::ostringstream& json, NvDsFrameMeta* frame) const {
    const PresenceEventMeta* event = eventMeta(frame);
    json << ",\"event\":";
    if (event == nullptr) {
        json << collector::JSON_NULL;
    } else {
        const unsigned count = slotCount(event);
        std::string label;
        bool firstName = true;
        bool alert = false;
        bool transit = false;
        json << "{\"event_code\":\"";
        for (unsigned index = 0; index < count; index++) {
            const EventSlot& slot = event->event.slots[index];
            json << slot.code;
            alert = alert || slot.code == eventmeta::ALERT;
            transit = transit || slot.code == eventmeta::TRANSIT;
            if (slot.code == eventmeta::ALERT && slot.name[0] != '\0') {
                if (!firstName) {
                    label.push_back(',');
                }
                firstName = false;
                label += slot.name;
            }
        }
        const char* level = alert ? eventmeta::LEVEL_ALERT : (transit ? eventmeta::LEVEL_TRANSIT : eventmeta::LEVEL_ABSENT);
        json << "\",\"level\":\"" << level
             << "\",\"event_label\":\"" << escapeLabel(label.c_str())
             << "\",\"debouncer\":{\"max_length\":" << event->debouncer.length
             << ",\"ratio_threshold\":" << std::fixed << std::setprecision(collector::CONF_DIGITS) << event->debouncer.threshold
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
                json << std::fixed << std::setprecision(collector::CONF_DIGITS) << slot.ratio;
            } else {
                json << collector::JSON_NULL;
            }
        }
        json << "]}}";
    }
}
