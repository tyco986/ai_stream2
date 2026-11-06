#include "base_generator/sahi_pose_rtsp.hpp"

#include "constants/pipeline_const.hpp"

#include <utility>

SahiPoseRtspGenerator::SahiPoseRtspGenerator(
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
    sahiPostprocess = "nvsahipostprocesspose";
}

YAML::Node SahiPoseRtspGenerator::sahiPostprocessProperties(const nlohmann::json& postprocess) {
    const std::string gieId = std::to_string(pgieGenerator->config["property"]["gie-unique-id"].as<int>());
    const YAML::Node properties = addNvsahipostprocessPose(
        gieId,
        false,
        postprocess.value("oks_threshold", pipeline::OKS_THRESHOLD),
        postprocess.value("vis_threshold", 0.0),
        postprocess.value("num_keypoints", 0));
    return properties;
}
