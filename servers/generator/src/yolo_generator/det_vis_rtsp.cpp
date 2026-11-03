#include "yolo_generator/det_vis_rtsp.hpp"

#include "subelement_generator/utils/default_gie/yolo_det.hpp"

#include <utility>

DetVisRtspGenerator::DetVisRtspGenerator(
    std::string pipelineName,
    nlohmann::json streams,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : VisRtspGenerator(
          std::move(pipelineName),
          std::move(streams),
          std::move(analyzer),
          std::move(pgie),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)) {
    generator = "DetVisRTSPGenerator";
}

void DetVisRtspGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloDet();
    pgieGenerator->updateConfig();
}
