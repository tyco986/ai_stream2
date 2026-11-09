#include "base_generator/sahi_pose_video.hpp"

#include "constants/pipeline_const.hpp"

#include <utility>

SahiPoseVideoGenerator::SahiPoseVideoGenerator(
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
    sahiPostprocess = SAHI_POSE_POSTPROCESS;
}

YAML::Node SahiPoseVideoGenerator::sahiPostprocessProperties(const nlohmann::json& postprocess) {
    const std::string gieId = std::to_string(pgieGenerator->config["property"]["gie-unique-id"].as<int>());
    const YAML::Node properties = addNvsahipostprocessPose(
        gieId,
        postprocess.value("class_agnostic", false),
        postprocess.value("match_metric", pipeline::MATCH_METRIC_IOS),
        postprocess.value("match_threshold", pipeline::MATCH_THRESHOLD),
        postprocess.value("prefer_full_frame", true),
        postprocess.value("max_detections", -1),
        postprocess.value("oks_threshold", pipeline::OKS_THRESHOLD),
        postprocess.value("vis_threshold", 0.0),
        postprocess.value("num_keypoints", 0));
    return properties;
}
