#pragma once

#include <vector>

#include "gstnvdsmeta.h"
#include "nvdsinfer.h"
#include "nvdspreprocess_meta.h"
#include "gstnvdsinfer.h"
#include "plugins/nvdspreprocess/rtmposepreprocess/engine/rect_expand.hpp"

class RtmposePostprocess {
public:
    explicit RtmposePostprocess(int sgieUniqueId, int inferWidth, int inferHeight, float padding);

    void setSgieUniqueId(int sgieUniqueId);
    int sgieUniqueId() const;
    void setGeometry(int inferWidth, int inferHeight, float padding);
    int inferWidth() const;
    int inferHeight() const;
    float padding() const;
    void apply(NvDsBatchMeta* batch);

private:
    struct Keypoints {
        const float* data;
        int joints;
    };

    bool batchTargetsSgie(const GstNvDsPreProcessBatchMeta* preprocessBatch) const;
    const NvDsInferTensorMeta* tensorFromRoi(NvDsRoiMeta* roi) const;
    Keypoints keypointsFromTensor(const NvDsInferTensorMeta* tensorMeta);
    void writeMaskParams(NvDsObjectMeta* object, const CropGeom& crop, const float* cropXyScore, int numJoints) const;
    void mirrorSnapshot(NvDsFrameMeta* frame, NvDsObjectMeta* object) const;
    void processRoi(NvDsRoiMeta* roi);

    int uniqueId;
    RectExpand expand;
    std::vector<float> hostScratch;
};
