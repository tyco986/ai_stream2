#pragma once

#include "base_generator/base_rtsp.hpp"
#include "topdown_pose_generator/topdown_pose.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include <optional>

// Generate ST-GCN++ RTMPose RTSP pipeline (headless).
//
// PGIE detects people. SGIE0 is RTMPose. SGIE1 is ST-GCN++.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvbboxsnapshot → nvtracker → nvdspreprocess_rtmpose → sgie0
//         → nvrtmposepostprocess → nvdspreprocess → sgie1 → nvstgcnppactionlabel → nvdsanalytics → tee_msg
//           ─┬→ nvstreamdemux → queue_demux{N} → nvstgcnpplogger{N} → fakesink{N}
//           └→ queue_msg → nvmsgconv → nvmsgbroker
class StgcnppRtmposeRtspGenerator : public BaseRtspGenerator {
public:
    StgcnppRtmposeRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sgie,
        nlohmann::json stgcnpp,
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
