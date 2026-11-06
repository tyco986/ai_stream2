#pragma once

#include "base_generator/vis_sahi_video.hpp"

// Generate YOLO SAHI segmentation video pipeline YAML with OSD and one mp4 per input.
//
// Drawer is segfadedrawer. OSD displays instance masks.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → boxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ segfadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue_demux{N} → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class SegSahiVisVideoGenerator : public VisSahiVideoGenerator {
public:
    SegSahiVisVideoGenerator(
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
    void add() override;
    void link() override;
};
