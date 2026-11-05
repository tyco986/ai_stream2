#include "plugins/fadedrawer/pose_fade_engine_with_tracker.hpp"

PoseFadeEngineWithTracker::PoseFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showPose, float poseThreshold, bool showSnap)
    : DetFadeEngine(interval, fadeTime, showLabel),
      PoseFadeEngine(interval, fadeTime, showLabel, showPose, poseThreshold),
      DetFadeEngineWithTracker(interval, fadeTime, showLabel, showSnap) {
}

void PoseFadeEngineWithTracker::decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) {
    (void)batch;
    (void)frame;
    (void)alpha;
    clearMask(object);
}
