#include "yolo_generator/det_vis_video.hpp"

#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

DetVisVideoGenerator::DetVisVideoGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : VisVideoGenerator(
          std::move(pipelineName),
          std::move(videos),
          std::move(output),
          std::move(analyzer),
          std::move(pgie),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)) {
    generator = "DetVisVideoGenerator";
}

void DetVisVideoGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}
