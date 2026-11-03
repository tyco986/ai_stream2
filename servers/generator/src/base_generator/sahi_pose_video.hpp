#pragma once

#include "base_generator/sahi_video.hpp"

// Generate YOLO SAHI pose video pipeline YAML (headless, ends at fakesink).
//
// Postprocess element is nvsahipostprocess_pose.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess_pose
//         → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ nvdetlogger → fakesink
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class SahiPoseVideoGenerator : public SahiVideoGenerator {
public:
    static constexpr const char* SAHI_POSE_POSTPROCESS = "nvsahipostprocess_pose";

    SahiPoseVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

protected:
    YAML::Node sahiPostprocessProperties(const nlohmann::json& postprocess) override;
};
