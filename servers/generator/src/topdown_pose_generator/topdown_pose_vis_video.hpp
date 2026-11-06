#pragma once

#include "base_generator/vis_video.hpp"
#include "topdown_pose_generator/topdown_pose.hpp"

#include <optional>

// Generate topdown-pose video pipeline with OSD and one mp4 per input.
//
// Drawer is posefadedrawer. OSD displays bbox and text.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdspreprocess_rtmpose → sgie0
//         → nvrtmposepostprocess → nvdsanalytics → tee_msg
//         ├→ posefadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue{N} → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class TopdownPoseVisVideoGenerator : public VisVideoGenerator {
public:
    TopdownPoseVisVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
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
