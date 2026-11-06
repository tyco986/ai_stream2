#pragma once

#include "base_generator/vis_rtsp.hpp"
#include "topdown_pose_generator/topdown_pose.hpp"

#include <optional>

// Generate topdown-pose RTSP pipeline with OSD preview.
//
// Drawer is posefadedrawer. OSD displays bbox and text.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdspreprocess_rtmpose → sgie0
//         → nvrtmposepostprocess → nvdsanalytics → tee_msg
//           ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → posefadedrawer{N} → nvosdbin{N}
//           │     → queue_enc{N} → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//           └→ queue_msg → nvmsgconv → nvmsgbroker
class TopdownPoseVisRtspGenerator : public VisRtspGenerator {
public:
    TopdownPoseVisRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sgie,
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
    std::optional<TopdownPose> pose;
};
