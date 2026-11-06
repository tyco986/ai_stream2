#include "plugins/gie/yolopose/engine/yolo_pose_parser.hpp"

extern "C" bool NvInferYoloPose(
    std::vector<NvDsInferLayerInfo> const& outputLayersInfo,
    NvDsInferNetworkInfo const& networkInfo,
    NvDsInferParseDetectionParams const& detectionParams,
    std::vector<NvDsInferInstanceMaskInfo>& objectList) {
    const YoloPoseParser parser(outputLayersInfo, networkInfo, detectionParams);
    const bool parsed = parser.parse(objectList);
    return parsed;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"
CHECK_CUSTOM_INSTANCE_MASK_PARSE_FUNC_PROTOTYPE(NvInferYoloPose);
#pragma GCC diagnostic pop
