#pragma once

#include "base_generator/vis_sahi_pose_video.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include <optional>

// Generate ST-GCN++ YOLO-pose SAHI video pipeline with OSD and one mp4 per input.
//
// Postprocess element is nvsahipostprocesspose. Drawer is stgcnppfadedrawerwithtracker.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocesspose
//         → boxsnapshot → nvtracker → nvdspreprocess → sgie1 → stgcnppactionlabel
//         → nvdsanalytics → nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → stgcnppfadedrawerwithtracker{N} → nvosdbin{N}
//         │     → stgcnpplogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
class StgcnppYoloPoseSahiVisVideoGenerator : public VisSahiPoseVideoGenerator {
public:
    StgcnppYoloPoseSahiVisVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
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
