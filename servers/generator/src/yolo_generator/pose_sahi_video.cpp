#include "yolo_generator/pose_sahi_video.hpp"

#include "subelement_generator/utils/default_gie/yolo_pose.hpp"

#include <utility>

PoseSahiVideoGenerator::PoseSahiVideoGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : SahiPoseVideoGenerator(
          std::move(pipelineName),
          std::move(videos),
          std::move(analyzer),
          std::move(pgie),
          std::move(sahi),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)) {
    generator = "PoseSahiVideoGenerator";
}

void PoseSahiVideoGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloPose();
    pgieGenerator->updateConfig();
}
