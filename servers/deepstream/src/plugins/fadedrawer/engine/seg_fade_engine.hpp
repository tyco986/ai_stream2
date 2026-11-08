#pragma once

#include "plugins/fadedrawer/engine/det_fade_engine.hpp"
#include "plugins/fadedrawer/engine/seg_mask.hpp"

// Detection fade plus an optional instance mask.
// Mask pixels stay on the object meta for nvdsosd unless showMask is false.
class SegFadeEngine : public DetFadeEngine {
public:
    SegFadeEngine(int interval, int fadeTime, bool showLabel, bool showMask);

protected:
    void decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) override;
    void decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha) override;
    bool keepMask() const override;

private:
    SegMask seg;
};
