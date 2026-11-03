#pragma once

#include "base_generator/vis_video.hpp"
#include "topdown_pose_generator/topdown_pose.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include <optional>

// Generate ST-GCN++ RTMPose video pipeline with OSD and one mp4 per input.
//
// Drawer is nvstgcnppfadedrawerwithtracker. OSD displays bbox and text.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvbboxsnapshot → nvtracker → nvdspreprocess_rtmpose → sgie0
//         → nvrtmposepostprocess → nvdspreprocess → sgie1 → nvstgcnppactionlabel → nvdsanalytics → tee_msg
//         ├→ nvstgcnppfadedrawerwithtracker → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue{N} → nvstgcnpplogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class StgcnppRtmposeVisVideoGenerator : public VisVideoGenerator {
public:
    StgcnppRtmposeVisVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
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
