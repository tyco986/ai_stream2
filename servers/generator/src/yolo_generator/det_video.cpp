#include "yolo_generator/det_video.hpp"

#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

DetVideoGenerator::DetVideoGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : BaseVideoGenerator(
          std::move(pipelineName),
          std::move(videos),
          std::move(analyzer),
          std::move(pgie),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)) {
    generator = "DetVideoGenerator";
}

void DetVideoGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}
