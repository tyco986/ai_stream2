#pragma once

#include <vector>

#include "nvdsinfer_custom_impl.h"

class YoloPoseParser {
public:
    YoloPoseParser(
        const std::vector<NvDsInferLayerInfo>& layers,
        const NvDsInferNetworkInfo& network,
        const NvDsInferParseDetectionParams& params);

    bool parse(std::vector<NvDsInferInstanceMaskInfo>& objects) const;

private:
    const NvDsInferLayerInfo* outputLayer() const;
    bool readShape(const NvDsInferLayerInfo& layer, unsigned int& numDetections, unsigned int& columns, unsigned int& keypointOffset, unsigned int& numKeypoints) const;
    float clip(float value, float limit) const;

    const std::vector<NvDsInferLayerInfo>& layers;
    const NvDsInferNetworkInfo& network;
    const NvDsInferParseDetectionParams& params;
};
