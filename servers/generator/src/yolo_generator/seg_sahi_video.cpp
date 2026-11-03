#include "yolo_generator/seg_sahi_video.hpp"

#include "subelement_generator/utils/default_gie/yolo_seg.hpp"

#include <utility>

SegSahiVideoGenerator::SegSahiVideoGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : SahiVideoGenerator(
          std::move(pipelineName),
          std::move(videos),
          std::move(analyzer),
          std::move(pgie),
          std::move(sahi),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)) {
    generator = "SegSahiVideoGenerator";
}

void SegSahiVideoGenerator::applyPgieConfig() {
    pgieGenerator->config = default_gie::yoloSeg();
    pgieGenerator->updateConfig();
}
