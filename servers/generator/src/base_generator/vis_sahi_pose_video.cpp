#include "base_generator/vis_sahi_pose_video.hpp"

#include "constants/pipeline_const.hpp"

#include <utility>

VisSahiPoseVideoGenerator::VisSahiPoseVideoGenerator(
    std::string pipelineName,
    nlohmann::json videos,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    std::optional<nlohmann::json> tracker,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : VisSahiVideoGenerator(
          std::move(pipelineName),
          std::move(videos),
          std::move(output),
          std::move(analyzer),
          std::move(pgie),
          std::move(sahi),
          std::move(tracker),
          std::move(logger),
          std::move(drawer)) {
    sahiPostprocess = "nvsahipostprocesspose";
}

YAML::Node VisSahiPoseVideoGenerator::sahiPostprocessProperties(const nlohmann::json& postprocess) {
    const std::string gieId = std::to_string(pgieGenerator->config["property"]["gie-unique-id"].as<int>());
    const YAML::Node properties = addNvsahipostprocessPose(
        gieId,
        false,
        postprocess.value("oks_threshold", pipeline::OKS_THRESHOLD),
        postprocess.value("vis_threshold", 0.0),
        postprocess.value("num_keypoints", 0));
    return properties;
}

std::string VisSahiPoseVideoGenerator::visDrawerElement() const {
    const std::string element = nvposeDrawerElement();
    return element;
}

YAML::Node VisSahiPoseVideoGenerator::visDrawerProperties(const nlohmann::json& drawerNode) {
    const YAML::Node properties = nvposeDrawerProperties(drawerNode);
    return properties;
}
