#include "plugins/fadedrawer/engine/pose_fade_engine.hpp"

PoseFadeEngine::PoseFadeEngine(int interval, int fadeTime, bool showLabel, bool showPose, float poseThreshold)
    : DetFadeEngine(interval, fadeTime, showLabel),
      pose(showPose, poseThreshold) {
}

void PoseFadeEngine::decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha) {
    pose.drawCached(batch, frame, cached, alpha);
}

void PoseFadeEngine::decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) {
    pose.drawObject(batch, frame, object, alpha);
}

bool PoseFadeEngine::keepMask() const {
    return pose.keep();
}
