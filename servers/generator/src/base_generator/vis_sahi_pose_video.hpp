#pragma once

#include "base_generator/vis_sahi_video.hpp"

// Generate YOLO SAHI pose video pipeline YAML with OSD and one mp4 per input.
//
// Postprocess element is nvsahipostprocesspose. Drawer is posefadedrawer.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocesspose
//         → boxsnapshot → nvtracker → nvdsanalytics → nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → posefadedrawer{N} → nvosdbin{N}
//         │     → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
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
