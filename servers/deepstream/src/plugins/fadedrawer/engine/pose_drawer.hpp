#pragma once

#include <vector>

#include "gstnvdsmeta.h"
#include "plugins/fadedrawer/engine/det_fade_engine.hpp"

// COCO-17 skeleton drawn from keypoints stored in mask_params.
class PoseDrawer {
public:
    PoseDrawer(bool showPose, float poseThreshold);

    bool keep() const;
    void drawObject(NvDsBatchMeta* batch, NvDsFrameMeta* frame, NvDsObjectMeta* object, float alpha) const;
    void drawCached(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const DetFadeEngine::CachedObject& cached, float alpha) const;

private:
    std::vector<float> decodeKeypoints(const float* data, unsigned int widthDim, unsigned int heightDim, float left, float top, float boxWidth, float boxHeight) const;
    int clampCoord(float value, int limit) const;
    void drawPose(NvDsBatchMeta* batch, NvDsFrameMeta* frame, const std::vector<float>& keypoints, float alpha) const;
    void clearMask(NvDsObjectMeta* object) const;

    bool showPose;
    float poseThreshold;
};
