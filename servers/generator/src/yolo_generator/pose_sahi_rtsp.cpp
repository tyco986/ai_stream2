#include "yolo_generator/pose_sahi_rtsp.hpp"

#include "subelement_generator/utils/default_gie/yolo_pose.hpp"

#include <utility>

PoseSahiRtspGenerator::PoseSahiRtspGenerator(
    std::string pipelineName,
    nlohmann::json streams,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : SahiPoseRtspGenerator(
          std::move(pipelineName),
          std::move(streams),
          std::move(analyzer),
          std::move(pgie),
          std::move(sahi),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)) {
    generator = "PoseSahiRTSPGenerator";
}

void PoseSahiRtspGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloPose();
    pgieGenerator->updateConfig();
}
