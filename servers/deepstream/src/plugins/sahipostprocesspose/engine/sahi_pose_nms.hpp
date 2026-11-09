#pragma once

#include <string>
#include <unordered_set>
#include <vector>

#include "gstnvdsmeta.h"
#include "plugins/sahipostprocesspose/engine/pose_detection.hpp"
#include "plugins/sahipostprocesspose/engine/spatial_grid.hpp"

class SahiPoseNms {
public:
    SahiPoseNms(const char* gieIdsText, bool classAgnostic, int matchMetric, float matchThreshold, bool preferFullFrame, int maxDetections, float oksThreshold, float visThreshold, int numKeypoints);

    void suppress(NvDsFrameMeta* frame);

private:
    void parseGieIds();
    bool accepts(const NvDsObjectMeta* objectMeta) const;
    std::vector<float> keypointsOf(const NvDsObjectMeta* objectMeta) const;
    float overlap(const PoseDetection& left, const PoseDetection& right) const;
    float keypointVar(unsigned int index, unsigned int keypointCount) const;
    float oks(const PoseDetection& left, const PoseDetection& right) const;
    bool anchored(const PoseDetection& detection) const;
    void suppressAnchors(const std::vector<PoseDetection>& detections, std::vector<char>& suppressed) const;
    void suppressByOverlap(const SpatialGrid& grid, const std::vector<PoseDetection>& detections, std::vector<char>& suppressed, const std::vector<unsigned int>& indexes) const;
    void suppressByOks(const SpatialGrid& grid, const std::vector<PoseDetection>& detections, std::vector<char>& suppressed, const std::vector<unsigned int>& indexes) const;
    void capDetections(const std::vector<PoseDetection>& detections, std::vector<char>& suppressed) const;

    std::string gieIdsText;
    bool classAgnostic;
    int matchMetric;
    float matchThreshold;
    bool preferFullFrame;
    int maxDetections;
    float oksThreshold;
    float visThreshold;
    int numKeypoints;
    bool filterAll;
    std::unordered_set<int> gieIds;
};
