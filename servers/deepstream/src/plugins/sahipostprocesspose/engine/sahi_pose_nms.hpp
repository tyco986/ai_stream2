#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "gstnvdsmeta.h"
#include "plugins/sahipostprocesspose/engine/pose_detection.hpp"
#include "plugins/sahipostprocesspose/engine/spatial_grid.hpp"

class SahiPoseNms {
public:
    SahiPoseNms(const char* gieIdsText, bool classAgnostic, float oksThreshold, float visThreshold, int numKeypoints);

    void suppress(NvDsFrameMeta* frame);

private:
    void parseGieIds();
    bool accepts(const NvDsObjectMeta* objectMeta) const;
    std::vector<float> keypointsOf(const NvDsObjectMeta* objectMeta) const;
    float keypointVar(unsigned int index, unsigned int keypointCount) const;
    float oks(const PoseDetection& left, const PoseDetection& right) const;
    void suppressGroup(const SpatialGrid& grid, std::vector<PoseDetection>& detections, std::vector<char>& suppressed, const std::vector<unsigned int>& indexes) const;

    std::string gieIdsText;
    bool classAgnostic;
    float oksThreshold;
    float visThreshold;
    int numKeypoints;
    bool filterAll;
    std::unordered_set<int> gieIds;
};
