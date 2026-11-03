#include "base_generator/sahi_pose_image.hpp"

#include "constants/pipeline_const.hpp"

#include <utility>

SahiPoseImageGenerator::SahiPoseImageGenerator(
    std::string pipelineName,
    nlohmann::json images,
    std::filesystem::path output,
    std::optional<nlohmann::json> analyzer,
    nlohmann::json pgie,
    nlohmann::json sahi,
    std::optional<nlohmann::json> logger,
    std::optional<nlohmann::json> drawer)
    : SahiImageGenerator(
          std::move(pipelineName),
          std::move(images),
          std::move(output),
          std::move(analyzer),
          std::move(pgie),
          std::move(sahi),
          std::move(logger),
          std::move(drawer)) {
    sahiPostprocess = "nvsahipostprocess_pose";
}

YAML::Node SahiPoseImageGenerator::sahiPostprocessProperties(const nlohmann::json& postprocess) {
    const std::string gieId = std::to_string(pgieGenerator->config["property"]["gie-unique-id"].as<int>());
    const YAML::Node properties = addNvsahipostprocessPose(
        gieId,
        false,
        postprocess.value("oks_threshold", pipeline::OKS_THRESHOLD),
        postprocess.value("vis_threshold", 0.0),
        postprocess.value("num_keypoints", 0));
    return properties;
}

std::string SahiPoseImageGenerator::visDrawerElement() const {
    const std::string element = "nvposefadedrawer";
    return element;
}

YAML::Node SahiPoseImageGenerator::visDrawerProperties(const nlohmann::json& drawerNode) {
    const YAML::Node properties = nvposeDrawerProperties(drawerNode);
    return properties;
}
