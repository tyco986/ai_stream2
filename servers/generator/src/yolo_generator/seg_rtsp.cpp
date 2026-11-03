#include "yolo_generator/seg_rtsp.hpp"

#include "subelement_generator/utils/default_gie/yolo_seg.hpp"

#include <utility>

SegRtspGenerator::SegRtspGenerator(
    std::string pipelineName,
    nlohmann::json streams,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : BaseRtspGenerator(
          std::move(pipelineName),
          std::move(streams),
          std::move(analyzer),
          std::move(pgie),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)) {
    generator = "SegRTSPGenerator";
}

void SegRtspGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloSeg();
    pgieGenerator->updateConfig();
}
