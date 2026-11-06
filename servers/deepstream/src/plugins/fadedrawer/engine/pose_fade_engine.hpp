#pragma once

#include <vector>

#include "plugins/fadedrawer/engine/det_fade_engine.hpp"

// Detection fade plus a COCO-17 skeleton.
// Keypoints live in mask_params as Kx3 (x, y, score), normalized to the box.
class PoseFadeEngine : public virtual DetFadeEngine {
public:
    PoseFadeEngine(int interval, int fadeTime, bool showLabel, bool showPose, float poseThreshold);

protected:
    void decorateObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) override;
    void decorateCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const CachedObject& cached, float alpha) override;
    bool keepMask() const override;
    void clearMask(NvDsObjectMeta* object) const;

private:
    std::vector<float> decodeKeypoints(const float* data, unsigned int widthDim, unsigned int heightDim, float left, float top, float boxWidth, float boxHeight) const;
    int clampCoord(float value, int limit) const;
    void drawPose(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const std::vector<float>& keypoints, float alpha);

    bool showPose;
    float poseThreshold;
};
