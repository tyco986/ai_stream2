#pragma once

#include "base_generator/vis_sahi_video.hpp"

// Generate YOLO SAHI pose video pipeline YAML with OSD and one mp4 per input.
//
// Postprocess element is nvsahipostprocess_pose. Drawer is nvposefadedrawer.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess_pose
//         → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ nvposefadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue_demux{N} → nvdetlogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class VisSahiPoseVideoGenerator : public VisSahiVideoGenerator {
public:
    VisSahiPoseVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

protected:
    YAML::Node sahiPostprocessProperties(const nlohmann::json& postprocess) override;
    std::string visDrawerElement() const override;
    YAML::Node visDrawerProperties(const nlohmann::json& drawer) override;
};
