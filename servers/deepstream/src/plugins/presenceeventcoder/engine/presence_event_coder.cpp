#include "plugins/presenceeventcoder/engine/presence_event_coder.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

extern "C" gpointer presenceEventMetaCopy(gpointer data, gpointer userData);
extern "C" void presenceEventMetaRelease(gpointer data, gpointer userData);

std::string trimmed(std::string text) {
    g_strstrip(text.data());
    return text.c_str();
}

std::vector<PresenceEventCoder::SlotConfig> PresenceEventCoder::parseEventSlotMap(const char* raw) const {
    std::vector<SlotConfig> parsed(eventmeta::SLOT_COUNT, SlotConfig{false, 0, "", ""});
    if (raw != nullptr && raw[0] != '\0') {
        gchar** tokens = g_strsplit(raw, ";", -1);
        for (guint index = 0; tokens[index] != nullptr; index++) {
            g_strstrip(tokens[index]);
            const std::string token = tokens[index];
            const std::size_t first = token.find(':');
            const std::size_t second = first == std::string::npos ? std::string::npos : token.find(':', first + 1);
            if (first != std::string::npos && second != std::string::npos) {
                const std::string slotText = trimmed(token.substr(0, first));
                const std::string classText = trimmed(token.substr(first + 1, second - first - 1));
                const std::string name = trimmed(token.substr(second + 1));
                const int slot = std::atoi(slotText.c_str());
                if (slot >= 0 && slot < eventmeta::SLOT_COUNT) {
                    parsed[static_cast<std::size_t>(slot)] = SlotConfig{true, std::atoi(classText.c_str()), name, name};
                }
            }
        }
        g_strfreev(tokens);
    }
    return parsed;
}

PresenceEventCoder::PresenceEventCoder(const char* eventSlotMap, int length, float threshold, const char* mode)
    : eventSlotMap(eventSlotMap != nullptr ? eventSlotMap : ""),
      length(length < 1 ? 1 : length),
      threshold(threshold),
      mode(mode != nullptr ? mode : presenceeventcoder::DEFAULT_MODE),
      slide(this->mode == "slide"),
      slots(parseEventSlotMap(this->eventSlotMap.c_str())),
      slotCount(0),
      pads() {
    for (unsigned int index = 0; index < slots.size(); index++) {
        if (slots[index].used) {
            slotCount = index + 1;
        }
    }
}

void PresenceEventCoder::rememberNames(const std::unordered_map<int, std::string>& labels) {
    for (SlotConfig& slot : slots) {
        const bool missing = slot.used && slot.eventName.empty() && slot.resolvedName.empty();
        const auto found = labels.find(slot.classId);
        if (missing && found != labels.end()) {
            slot.resolvedName = found->second;
        }
    }
}

void PresenceEventCoder::encode(PadState& pad, const std::vector<int>& detected, PresenceEventMeta* meta) {
    if (pad.tracks.size() != slots.size()) {
        pad.tracks.assign(slots.size(), ClassTrack{});
        for (ClassTrack& track : pad.tracks) {
            track.eventCode = eventmeta::ABSENT;
            track.ratio = 0.0f;
            track.filled = 0;
        }
        pad.hasLast = false;
        std::memset(&pad.lastMeta, 0, sizeof(pad.lastMeta));
    }
    for (std::size_t index = 0; index < slots.size(); index++) {
        ClassTrack& track = pad.tracks[index];
        if (slots[index].used) {
            const bool seen = std::find(detected.begin(), detected.end(), slots[index].classId) != detected.end();
            track.detectedWindow.push_back(seen ? 1 : 0);
            track.filled = static_cast<int>(track.detectedWindow.size());
            if (track.filled >= length) {
                int hits = 0;
                for (char flag : track.detectedWindow) {
                    hits += flag;
                }
                track.ratio = static_cast<float>(hits) / static_cast<float>(length);
                if (slide) {
                    track.detectedWindow.erase(track.detectedWindow.begin());
                } else {
                    track.detectedWindow.clear();
                }
            }
            if (!seen) {
                track.eventCode = eventmeta::ABSENT;
            } else if (track.filled >= length && track.ratio >= threshold) {
                track.eventCode = eventmeta::ALERT;
            } else {
                track.eventCode = eventmeta::TRANSIT;
            }
            track.codeWindow.push_back(track.eventCode);
            if (track.filled >= length && slide && !track.codeWindow.empty()) {
                track.codeWindow.erase(track.codeWindow.begin());
            } else if (track.filled >= length && !slide) {
                track.codeWindow.clear();
            }
        }
    }
    std::memset(meta, 0, sizeof(*meta));
    meta->event.slotCount = slotCount;
    meta->debouncer.length = length;
    meta->debouncer.threshold = threshold;
    for (unsigned int index = 0; index < slotCount; index++) {
        meta->event.slots[index].code = eventmeta::ABSENT;
        if (slots[index].used) {
            const ClassTrack& track = pad.tracks[index];
            meta->event.slots[index].code = track.eventCode;
            g_strlcpy(meta->event.slots[index].name, slots[index].resolvedName.c_str(), eventmeta::NAME_LEN);
            meta->debouncer.slots[index].classId = slots[index].classId;
            meta->debouncer.slots[index].ratio = track.ratio;
            meta->debouncer.slots[index].filled = track.filled;
            for (char code : track.codeWindow) {
                if (code >= eventmeta::ABSENT && code <= eventmeta::TRANSIT) {
                    meta->debouncer.slots[index].counts[code - eventmeta::ABSENT] += 1;
                }
            }
        }
    }
}

void PresenceEventCoder::attach(NvDsBatchMeta* batch, NvDsFrameMeta* frame) {
    std::vector<int> detected;
    std::unordered_map<int, std::string> labels;
    for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
        auto* object = static_cast<NvDsObjectMeta*>(item->data);
        if (object != nullptr) {
            detected.push_back(object->class_id);
            if (object->obj_label[0] != '\0' && labels.count(object->class_id) == 0) {
                labels.emplace(object->class_id, object->obj_label);
            }
        }
    }
    rememberNames(labels);
    auto* payload = static_cast<PresenceEventMeta*>(g_malloc0(sizeof(PresenceEventMeta)));
    PadState& pad = pads[static_cast<int>(frame->pad_index)];
    if (frame->bInferDone != 0 || !pad.hasLast) {
        encode(pad, detected, payload);
        pad.lastMeta = *payload;
        pad.hasLast = true;
    } else {
        *payload = pad.lastMeta;
    }
    NvDsMetaList* item = frame->frame_user_meta_list;
    while (item != nullptr) {
        NvDsMetaList* next = item->next;
        auto* userMeta = static_cast<NvDsUserMeta*>(item->data);
        if (userMeta != nullptr && userMeta->base_meta.meta_type == presenceeventcoder::USER_META) {
            nvds_remove_user_meta_from_frame(frame, userMeta);
        }
        item = next;
    }
    NvDsUserMeta* userMeta = nvds_acquire_user_meta_from_pool(batch);
    if (userMeta != nullptr) {
        userMeta->user_meta_data = payload;
        userMeta->base_meta.meta_type = presenceeventcoder::USER_META;
        userMeta->base_meta.copy_func = presenceEventMetaCopy;
        userMeta->base_meta.release_func = presenceEventMetaRelease;
        nvds_add_user_meta_to_frame(frame, userMeta);
    } else {
        release(payload);
    }
}

PresenceEventMeta* PresenceEventCoder::copy(const PresenceEventMeta* src) {
    PresenceEventMeta* dst = nullptr;
    if (src != nullptr) {
        dst = static_cast<PresenceEventMeta*>(g_malloc0(sizeof(PresenceEventMeta)));
        std::memcpy(dst, src, sizeof(PresenceEventMeta));
    }
    return dst;
}

void PresenceEventCoder::release(PresenceEventMeta* meta) {
    g_free(meta);
}

extern "C" gpointer presenceEventMetaCopy(gpointer data, gpointer userData) {
    (void)userData;
    const PresenceEventMeta* src = nullptr;
    auto* userMeta = static_cast<NvDsUserMeta*>(data);
    if (userMeta != nullptr) {
        src = static_cast<const PresenceEventMeta*>(userMeta->user_meta_data);
    }
    return PresenceEventCoder::copy(src);
}

extern "C" void presenceEventMetaRelease(gpointer data, gpointer userData) {
    (void)userData;
    auto* userMeta = static_cast<NvDsUserMeta*>(data);
    if (userMeta != nullptr) {
        PresenceEventCoder::release(static_cast<PresenceEventMeta*>(userMeta->user_meta_data));
        userMeta->user_meta_data = nullptr;
    }
}
