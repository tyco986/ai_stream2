#include "event_generator/det_sahi_rtsp_presence.hpp"

#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

DetSahiRtspPresenceGenerator::DetSahiRtspPresenceGenerator(
    std::string pipelineName,
    nlohmann::json streams,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    nlohmann::json eventCoder,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer,
    std::optional<nlohmann::json> capturer)
    : EventSahiRtspGenerator(
          std::move(pipelineName),
          std::move(streams),
          std::move(analyzer),
          std::move(pgie),
          std::move(sahi),
          std::move(eventCoder),
          std::move(tracker),
          std::move(logger),
          std::move(drawer),
          std::move(capturer)) {
    generator = "DetSahiRTSPPresenceGenerator";
}

void DetSahiRtspPresenceGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}
