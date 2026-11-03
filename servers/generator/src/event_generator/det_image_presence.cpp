#include "event_generator/det_image_presence.hpp"

#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

DetImagePresenceGenerator::DetImagePresenceGenerator(
    std::string pipelineName,
    nlohmann::json images,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json eventCoder,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer,
    std::optional<nlohmann::json> capturer)
    : EventImageGenerator(
          std::move(pipelineName),
          std::move(images),
          std::move(output),
          std::move(analyzer),
          std::move(pgie),
          std::move(eventCoder),
          std::move(logger),
          std::move(drawer),
          std::move(capturer)) {
    generator = "DetImagePresenceGenerator";
}

void DetImagePresenceGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}
