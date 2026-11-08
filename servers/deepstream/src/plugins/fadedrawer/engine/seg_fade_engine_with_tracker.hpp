#pragma once

#include "plugins/fadedrawer/engine/det_fade_engine_with_tracker.hpp"
#include "plugins/fadedrawer/engine/seg_mask.hpp"

class SegFadeEngineWithTracker : public DetFadeEngineWithTracker {
public:
    SegFadeEngineWithTracker(int interval, int fadeTime, bool showLabel, bool showMask, bool showSnap);

protected:
    void decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) override;
    void decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha) override;
    bool keepMask() const override;

private:
    SegMask seg;
};
