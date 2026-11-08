#pragma once

#include "gstnvdsmeta.h"
#include "plugins/fadedrawer/engine/det_fade_engine.hpp"

// Instance mask kept on object meta for nvdsosd.
class SegMask {
public:
    explicit SegMask(bool showMask);

    bool keep() const;
    void clear(NvDsObjectMeta* object) const;
    void clearUnlessShown(NvDsObjectMeta* object) const;
    void attach(DetFadeEngine& engine, NvDsBatchMeta* batch, NvDsFrameMeta* frame, const DetFadeEngine::CachedObject& cached, const NvOSD_ColorParams& border) const;

private:
    bool showMask;
};
