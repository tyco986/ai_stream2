#pragma once

#include "base_generator/sahi_pose_video.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include <optional>

// Generate ST-GCN++ YOLO-pose SAHI video pipeline (headless, ends at fakesink).
//
// mux batch size is the video count. pgie batch size is the SAHI tile count.
// Postprocess element is nvsahipostprocesspose.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocesspose
//         → boxsnapshot → nvtracker → nvdspreprocess → sgie1 → stgcnppactionlabel
//         → nvdsanalytics → tee_msg
//         ├→ stgcnpplogger → fakesink
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class StgcnppYoloPoseSahiVideoGenerator : public SahiPoseVideoGenerator {
public:
    StgcnppYoloPoseSahiVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json stgcnpp,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
    void initParams() override;
    void applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) override;
    void add() override;
    void link() override;

private:
    nlohmann::json stgcnpp;
    std::optional<StgcnppYoloPose> model;
};
