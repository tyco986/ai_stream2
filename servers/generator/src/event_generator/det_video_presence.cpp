#include "event_generator/det_video_presence.hpp"

#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

DetVideoPresenceGenerator::DetVideoPresenceGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json eventCoder,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer,
    std::optional<nlohmann::json> capturer)
    : EventVideoGenerator(
          std::move(pipelineName),
          std::move(videos),
          std::move(analyzer),
          std::move(pgie),
          std::move(eventCoder),
          std::move(tracker),
          std::move(logger),
          std::move(drawer),
          std::move(capturer)) {
    generator = "DetVideoPresenceGenerator";
}

void DetVideoPresenceGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}
