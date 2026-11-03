#pragma once

#include "base_generator/vis_video.hpp"

// Generate YOLO segmentation video pipeline YAML with OSD and one mp4 per input.
//
// Drawer is nvsegfadedrawer. OSD displays instance masks.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ nvsegfadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue{N} → nvdetlogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class SegVisVideoGenerator : public VisVideoGenerator {
public:
    SegVisVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
    void add() override;
    void link() override;
};
