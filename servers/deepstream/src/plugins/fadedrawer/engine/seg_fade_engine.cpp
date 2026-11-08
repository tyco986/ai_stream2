#include "plugins/fadedrawer/engine/seg_fade_engine.hpp"

SegFadeEngine::SegFadeEngine(int interval, int fadeTime, bool showLabel, bool showMask)
    : DetFadeEngine(interval, fadeTime, showLabel),
      seg(showMask) {
}

bool SegFadeEngine::keepMask() const {
    return seg.keep();
}

void SegFadeEngine::decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha) {
    seg.attach(*this, batch, frame, cached, boxColor(frame, alpha));
}

void SegFadeEngine::decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) {
    (void)batch;
    (void)frame;
    (void)alpha;
    seg.clearUnlessShown(object);
}
