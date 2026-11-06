#pragma once

#include "plugins/fadedrawer/engine/det_fade_engine_with_tracker.hpp"
#include "plugins/fadedrawer/engine/pose_fade_engine.hpp"

class PoseFadeEngineWithTracker : public PoseFadeEngine, public DetFadeEngineWithTracker {
public:
    PoseFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showPose, float poseThreshold, bool showSnap);

protected:
    void decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) override;
};
