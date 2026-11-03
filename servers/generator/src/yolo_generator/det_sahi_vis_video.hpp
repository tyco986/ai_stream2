#pragma once

#include "base_generator/vis_sahi_video.hpp"

// Generate YOLO SAHI detection video pipeline YAML with OSD and one mp4 per input.
//
// Drawer is nvdetfadedrawer. OSD displays bbox and text.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ nvdetfadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue_demux{N} → nvdetlogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
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
