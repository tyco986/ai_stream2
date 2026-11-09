#include "plugins/fadedrawer/engine/pose_fade_engine_with_tracker.hpp"

PoseFadeEngineWithTracker::PoseFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showPose, float poseThreshold, bool showSnap)
    : DetFadeEngineWithTracker(interval, fadeTime, showLabel, showSnap),
      pose(showPose, poseThreshold) {
}

void PoseFadeEngineWithTracker::decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) {
    (void)batch;
    (void)frame;
    (void)alpha;
    pose.clearObject(object);
}

void PoseFadeEngineWithTracker::decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha) {
    pose.drawCached(batch, frame, cached, alpha);
}

bool PoseFadeEngineWithTracker::keepMask() const {
    return pose.keep();
}
