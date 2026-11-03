#include "yolo_generator/pose_sahi_image.hpp"

#include "subelement_generator/utils/default_gie/yolo_pose.hpp"

#include <utility>

PoseSahiImageGenerator::PoseSahiImageGenerator(
    std::string pipelineName,
    nlohmann::json images,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : SahiPoseImageGenerator(
          std::move(pipelineName),
          std::move(images),
          std::move(output),
          std::move(analyzer),
          std::move(pgie),
          std::move(sahi),
          std::move(logger),
          std::move(drawer)) {
    generator = "PoseSahiImageGenerator";
}

void PoseSahiImageGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloPose();
    pgieGenerator->updateConfig();
}
