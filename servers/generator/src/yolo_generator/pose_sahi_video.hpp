#pragma once

#include "base_generator/sahi_pose_video.hpp"

// Generate YOLO SAHI pose video pipeline YAML (headless, ends at fakesink).
//
// mux batch size is the number of videos. pgie batch size is the SAHI tile count.
// Postprocess element is nvsahipostprocess_pose.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess_pose
//         → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ nvdetlogger → fakesink
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class PoseSahiVideoGenerator : public SahiPoseVideoGenerator {
public:
    PoseSahiVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
