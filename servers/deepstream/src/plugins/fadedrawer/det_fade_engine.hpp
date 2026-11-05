#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "constants/fadedrawer_const.hpp"
#include "gstnvdsmeta.h"

// Fade detection boxes between inference frames.
//
// An inference frame is cached per pad and drawn at full opacity. Later frames
// replay that cache as new object meta, with border and label alpha taken from
// a triangle wave. Presence event codes select the border color: '1' red,
// '2' yellow, otherwise green.
class DetFadeEngine {
public:
    DetFadeEngine(int interval, int fadeTime, bool showLabel);
    virtual ~DetFadeEngine() = default;

    virtual void processFrame(NvDsBatchMeta* batch, NvDsFrameMeta* frame);
    virtual void dropStream(int padIndex);

protected:
    struct CachedObject {
        float left = 0.0f;
        float top = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float confidence = 0.0f;
        int classId = 0;
        std::uint64_t objectId = ~0ull;
        std::string label;
        std::vector<float> mask;
        unsigned int maskSize = 0;
        unsigned int maskWidth = 0;
        unsigned int maskHeight = 0;
        float maskThreshold = 0.0f;
    };

    struct StreamState {
        int phase = 0;
        std::vector<CachedObject> objects;
    };

    virtual void writeLabel(NvDsObjectMeta* object) const;
    virtual void decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha);
    virtual void decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha);
    virtual bool keepMask() const;
    static NvOSD_ColorParams colorWithAlpha(const float rgb[4], float alpha);
    NvOSD_ColorParams boxColor(NvDsFrameMeta* frame, float alpha) const;
    void paint(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, const NvOSD_ColorParams& border, float alpha);
    void cacheMask(CachedObject& cached, const NvOSD_MaskParams& mask) const;
    void restoreMask(NvDsObjectMeta* object, const CachedObject& cached) const;

    std::unordered_map<int, StreamState> streams;
    std::vector<float> alphaLut;
    bool showLabel;

private:
    struct PresenceEventMeta {
        char eventCodes[fadedrawer::PRESENCE_EVENT_CODE_LEN + 1];
        char eventNames[fadedrawer::PRESENCE_EVENT_CODE_LEN][64];
        unsigned int numClasses;
    };

    static const PresenceEventMeta* presenceEvent(NvDsFrameMeta* frame);
    static const float* borderRgb(NvDsFrameMeta* frame);
    std::vector<float> buildLut(int interval, int fadeTime) const;
    void renewCache(StreamState& state, NvDsFrameMeta* frame) const;
    void replay(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const StreamState& state, const NvOSD_ColorParams& border, float alpha);

    int interval;
    int fadeTime;
};
