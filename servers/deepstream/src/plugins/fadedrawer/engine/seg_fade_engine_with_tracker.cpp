#include "plugins/fadedrawer/engine/seg_fade_engine_with_tracker.hpp"

SegFadeEngineWithTracker::SegFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showMask, bool showSnap)
    : DetFadeEngineWithTracker(interval, fadeTime, showLabel, showSnap),
      seg(showMask) {
}

void SegFadeEngineWithTracker::decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) {
    (void)batch;
    (void)frame;
    (void)alpha;
    seg.clear(object);
}

void SegFadeEngineWithTracker::decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha) {
    seg.attach(*this, batch, frame, cached, boxColor(frame, alpha));
}

bool SegFadeEngineWithTracker::keepMask() const {
    return seg.keep();
}
