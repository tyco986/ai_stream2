#pragma once

#include "plugins/fadedrawer/engine/det_fade_engine.hpp"
#include "plugins/fadedrawer/engine/pose_drawer.hpp"

// Detection fade plus a COCO-17 skeleton.
// Keypoints live in mask_params as Kx3 (x, y, score), normalized to the box.
class PoseFadeEngine : public DetFadeEngine {
public:
    PoseFadeEngine(int interval, int fadeTime, bool showLabel, bool showPose, float poseThreshold);

protected:
    void decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) override;
    void decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha) override;
    bool keepMask() const override;

private:
    PoseDrawer pose;
};
