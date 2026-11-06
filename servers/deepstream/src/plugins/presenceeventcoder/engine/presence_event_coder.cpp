#include "plugins/presenceeventcoder/engine/presence_event_coder.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

extern "C" gpointer presenceEventMetaCopy(gpointer data, gpointer userData);
extern "C" void presenceEventMetaRelease(gpointer data, gpointer userData);

std::vector<PresenceEventCoder::ClassEvent> PresenceEventCoder::parseClassEventMap(const char* raw) const {
    std::vector<ClassEvent> events;
    if (raw != nullptr && raw[0] != '\0') {
        gchar** tokens = g_strsplit(raw, ";", -1);
        for (guint index = 0; tokens[index] != nullptr && events.size() < static_cast<std::size_t>(presenceeventcoder::CODE_LEN); index++) {
            g_strstrip(tokens[index]);
            const std::string token = tokens[index];
            const std::size_t colon = token.find(':');
            if (colon != std::string::npos) {
                std::string classId = token.substr(0, colon);
                std::string name = token.substr(colon + 1);
                g_strstrip(classId.data());
                g_strstrip(name.data());
                events.push_back(ClassEvent{std::atoi(classId.c_str()), name.c_str()});
            }
        }
        g_strfreev(tokens);
    }
    return events;
}

PresenceEventCoder::PresenceEventCoder(const char* classEventMap, int length, float threshold, const char* mode)
    : classEventMap(classEventMap != nullptr ? classEventMap : ""),
      length(length < 1 ? 1 : length),
      threshold(threshold),
      mode(mode != nullptr ? mode : presenceeventcoder::DEFAULT_MODE),
      slide(this->mode == "slide"),
      classEvents(parseClassEventMap(this->classEventMap.c_str())),
      pads() {
}

void PresenceEventCoder::encode(PadState& pad, const std::vector<int>& detected, PresenceEventMeta* meta) {
    if (pad.tracks.size() != classEvents.size()) {
        pad.tracks.assign(classEvents.size(), ClassTrack{});
        for (ClassTrack& track : pad.tracks) {
            track.eventCode = presenceeventcoder::ABSENT;
            track.ratio = 0.0f;
        }
        pad.hasLast = false;
        std::memset(&pad.lastMeta, 0, sizeof(pad.lastMeta));
    }
    for (std::size_t index = 0; index < classEvents.size(); index++) {
        ClassTrack& track = pad.tracks[index];
        const bool seen = std::find(detected.begin(), detected.end(), classEvents[index].classId) != detected.end();
        track.detectedWindow.push_back(seen ? 1 : 0);
        if (static_cast<int>(track.detectedWindow.size()) >= length) {
            int hits = 0;
            for (char flag : track.detectedWindow) {
                hits += flag;
            }
            track.ratio = static_cast<float>(hits) / static_cast<float>(length);
            if (track.ratio >= threshold) {
                track.eventCode = seen ? presenceeventcoder::ALERT : presenceeventcoder::TRANSIT;
            } else {
                track.eventCode = seen ? presenceeventcoder::TRANSIT : presenceeventcoder::ABSENT;
            }
            track.codeWindow.push_back(track.eventCode);
            if (slide) {
                track.detectedWindow.erase(track.detectedWindow.begin());
                if (!track.codeWindow.empty()) {
                    track.codeWindow.erase(track.codeWindow.begin());
                }
            } else {
                track.detectedWindow.clear();
                track.codeWindow.clear();
            }
        } else {
            track.eventCode = seen ? presenceeventcoder::TRANSIT : presenceeventcoder::ABSENT;
            track.codeWindow.push_back(track.eventCode);
        }
    }
    std::memset(meta, 0, sizeof(*meta));
    for (int index = 0; index < presenceeventcoder::CODE_LEN; index++) {
        meta->eventCodes[index] = presenceeventcoder::ABSENT;
    }
    meta->numClasses = static_cast<unsigned int>(classEvents.size());
    for (unsigned int index = 0; index < meta->numClasses; index++) {
        const ClassTrack& track = pad.tracks[index];
        meta->eventCodes[index] = track.eventCode;
        meta->classes[index].classId = classEvents[index].classId;
        meta->classes[index].ratio = track.ratio;
        for (char code : track.codeWindow) {
            if (code >= presenceeventcoder::ABSENT && code <= presenceeventcoder::TRANSIT) {
                meta->classes[index].counts[code - presenceeventcoder::ABSENT] += 1;
            }
        }
        if (track.eventCode == presenceeventcoder::ALERT) {
            g_strlcpy(meta->eventNames[index], classEvents[index].name.c_str(), presenceeventcoder::NAME_LEN);
        }
    }
}

void PresenceEventCoder::attach(NvDsBatchMeta* batch, NvDsFrameMeta* frame) {
    std::vector<int> detected;
    for (NvDsMetaList* item = frame->obj_meta_list; item != nullptr; item = item->next) {
        auto* object = static_cast<NvDsObjectMeta*>(item->data);
        if (object != nullptr) {
            detected.push_back(object->class_id);
        }
    }
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
