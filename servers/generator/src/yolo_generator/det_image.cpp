#include "yolo_generator/det_image.hpp"

#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

DetImageGenerator::DetImageGenerator(
    std::string pipelineName,
    nlohmann::json images,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : BaseImageGenerator(
          std::move(pipelineName),
          std::move(images),
          std::move(output),
          std::move(analyzer),
          std::move(pgie),
          std::move(logger),
          std::move(drawer)) {
    generator = "DetImageGenerator";
}

void DetImageGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}
