#pragma once

#include <vector>

#include "gstnvdsmeta.h"

struct PoseDetection {
    float left;
    float top;
    float right;
    float bottom;
    float score;
    int classId;
    float area;
    NvDsObjectMeta* objectMeta;
    std::vector<float> keypoints;
};
