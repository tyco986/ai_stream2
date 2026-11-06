#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "constants/presenceeventcoder_const.hpp"
#include "gstnvdsmeta.h"

struct PresenceClassStat {
    int classId;
    unsigned int counts[presenceeventcoder::CODE_BOUNDS];
    float ratio;
};

struct PresenceEventMeta {
    char eventCodes[presenceeventcoder::CODE_LEN + 1];
    char eventNames[presenceeventcoder::CODE_LEN][presenceeventcoder::NAME_LEN];
    unsigned int numClasses;
    PresenceClassStat classes[presenceeventcoder::CODE_LEN];
};

class PresenceEventCoder {
public:
    // Ordered map from a model class id to its event name.
    // Semicolon-separated; class id before the colon, event name after.
    // Order is the event-code index, at most 8 entries. Example: "0:smoke;1:fire"
    PresenceEventCoder(const char* classEventMap, int length, float threshold, const char* mode);

    void attach(NvDsBatchMeta* batch, NvDsFrameMeta* frame);

    static PresenceEventMeta* copy(const PresenceEventMeta* src);
    static void release(PresenceEventMeta* meta);

private:
    struct ClassTrack {
        std::vector<char> detectedWindow;
        std::vector<char> codeWindow;
        char eventCode;
        float ratio;
    };

    struct ClassEvent {
        int classId;
        std::string name;
    };

    struct PadState {
        std::vector<ClassTrack> tracks;
        PresenceEventMeta lastMeta;
        bool hasLast;
    };

    std::vector<ClassEvent> parseClassEventMap(const char* raw) const;
    void encode(PadState& pad, const std::vector<int>& detected, PresenceEventMeta* meta);

    std::string classEventMap;
    int length;
    float threshold;
    std::string mode;
    bool slide;
    std::vector<ClassEvent> classEvents;
    std::unordered_map<int, PadState> pads;
};
