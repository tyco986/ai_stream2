#pragma once

#include "base_generator/vis_rtsp.hpp"
#include "topdown_pose_generator/topdown_pose.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include <optional>

// Generate ST-GCN++ RTMPose RTSP pipeline with OSD preview.
//
// Drawer is nvstgcnppfadedrawerwithtracker. OSD displays bbox and text.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvbboxsnapshot → nvtracker → nvdspreprocess_rtmpose → sgie0
//         → nvrtmposepostprocess → nvdspreprocess → sgie1 → nvstgcnppactionlabel → nvdsanalytics → tee_msg
//           ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → nvstgcnppfadedrawerwithtracker{N} → nvosdbin{N}
//           │     → queue_enc{N} → nvstgcnpplogger{N} → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//           └→ queue_msg → nvmsgconv → nvmsgbroker
class StgcnppRtmposeVisRtspGenerator : public VisRtspGenerator {
public:
    StgcnppRtmposeVisRtspGenerator(
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
