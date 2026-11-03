#include "event_generator/det_sahi_vis_rtsp_presence.hpp"

#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

DetSahiVisRtspPresenceGenerator::DetSahiVisRtspPresenceGenerator(
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
    : VisEventSahiRtspGenerator(
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
    generator = "DetSahiVisRTSPPresenceGenerator";
}

void DetSahiVisRtspPresenceGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}
