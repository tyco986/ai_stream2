#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "constants/presenceeventcoder_const.hpp"
#include "gstnvdsmeta.h"
#include "plugins/eventmeta/event_meta.hpp"

struct PresenceDebouncerSlot {
    int classId;
    unsigned int counts[presenceeventcoder::CODE_BOUNDS];
    float ratio;
    int filled;
};

struct PresenceDebouncer {
    int length;
    float threshold;
    PresenceDebouncerSlot slots[eventmeta::SLOT_COUNT];
};

struct PresenceEventMeta {
    EventMeta event;
    PresenceDebouncer debouncer;
};

class PresenceEventCoder {
public:
    // Slot to model class and event name. Semicolon-separated.
    // Slot, class id, then name. Name may be empty. At most 8 slots.
    // Example: "0:0:Fire;1:1:Smoke"
    PresenceEventCoder(const char* eventSlotMap, int length, float threshold, const char* mode);

    void attach(NvDsBatchMeta* batch, NvDsFrameMeta* frame);

    static PresenceEventMeta* copy(const PresenceEventMeta* src);
    static void release(PresenceEventMeta* meta);

private:
    struct SlotConfig {
        bool used;
        int classId;
        std::string eventName;
        std::string resolvedName;
    };

    struct ClassTrack {
        std::vector<char> detectedWindow;
        std::vector<char> codeWindow;
        char eventCode;
        float ratio;
        int filled;
    };

    struct PadState {
        std::vector<ClassTrack> tracks;
        PresenceEventMeta lastMeta;
        bool hasLast;
    };

    std::vector<SlotConfig> parseEventSlotMap(const char* raw) const;
    void rememberNames(const std::unordered_map<int, std::string>& labels);
    void encode(PadState& pad, const std::vector<int>& detected, PresenceEventMeta* meta);

    std::string eventSlotMap;
    int length;
    float threshold;
    std::string mode;
    bool slide;
    std::vector<SlotConfig> slots;
    unsigned int slotCount;
    std::unordered_map<int, PadState> pads;
};
