#include "yolo_generator/seg_sahi_rtsp.hpp"

#include "subelement_generator/utils/default_gie/yolo_seg.hpp"

#include <utility>

SegSahiRtspGenerator::SegSahiRtspGenerator(
    std::string pipelineName,
    nlohmann::json streams,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : SahiRtspGenerator(
          std::move(pipelineName),
          std::move(streams),
          std::move(analyzer),
          std::move(pgie),
          std::move(sahi),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)) {
    generator = "SegSahiRTSPGenerator";
}

void SegSahiRtspGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloSeg();
    pgieGenerator->updateConfig();
}
