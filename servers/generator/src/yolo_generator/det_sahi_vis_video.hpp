#pragma once

#include "base_generator/vis_sahi_video.hpp"

// Generate YOLO SAHI detection video pipeline YAML with OSD and one mp4 per input.
//
// Drawer is detfadedrawer. OSD displays bbox and text.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → boxsnapshot → nvtracker → nvdsanalytics → nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → detfadedrawer{N} → nvosdbin{N}
//         │     → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
class DetSahiVisVideoGenerator : public VisSahiVideoGenerator {
public:
    DetSahiVisVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
