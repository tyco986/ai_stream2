#pragma once

#include "base_generator/sahi_video.hpp"
#include "topdown_pose_generator/topdown_pose.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include <optional>

// Generate ST-GCN++ RTMPose SAHI video pipeline (headless, ends at fakesink).
//
// mux batch size is the video count. pgie batch size is the SAHI tile count.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → nvbboxsnapshot → nvtracker → nvdspreprocess_rtmpose → sgie0 → nvrtmposepostprocess
//         → nvdsanalytics → tee_msg
//         ├→ nvstgcnpplogger → fakesink
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class StgcnppRtmposeSahiVideoGenerator : public SahiVideoGenerator {
public:
    StgcnppRtmposeSahiVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sgie,
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
    nlohmann::json sgie;
    nlohmann::json stgcnpp;
    std::optional<TopdownPose> pose;
    std::optional<StgcnppYoloPose> model;
};
