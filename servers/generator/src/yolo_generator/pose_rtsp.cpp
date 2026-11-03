#include "yolo_generator/pose_rtsp.hpp"

#include "subelement_generator/utils/default_gie/yolo_pose.hpp"

#include <utility>

PoseRtspGenerator::PoseRtspGenerator(
    std::string pipelineName,
    nlohmann::json streams,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : BaseRtspGenerator(
          std::move(pipelineName),
          std::move(streams),
          std::move(analyzer),
          std::move(pgie),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)) {
    generator = "PoseRTSPGenerator";
}

void PoseRtspGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloPose();
    pgieGenerator->updateConfig();
}
