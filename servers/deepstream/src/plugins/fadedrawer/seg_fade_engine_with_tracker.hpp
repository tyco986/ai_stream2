#pragma once

#include "plugins/fadedrawer/det_fade_engine_with_tracker.hpp"
#include "plugins/fadedrawer/seg_fade_engine.hpp"

class SegFadeEngineWithTracker : public SegFadeEngine, public DetFadeEngineWithTracker {
public:
    SegFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showMask, bool showSnap);

protected:
    void decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) override;
};
