#include "plugins/fadedrawer/engine/seg_fade_engine_with_tracker.hpp"

SegFadeEngineWithTracker::SegFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showMask, bool showSnap)
    : DetFadeEngine(interval, fadeTime, showLabel),
      SegFadeEngine(interval, fadeTime, showLabel, showMask),
      DetFadeEngineWithTracker(interval, fadeTime, showLabel, showSnap) {
}

void SegFadeEngineWithTracker::decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) {
    (void)batch;
    (void)frame;
    (void)alpha;
    clearMask(object);
}
