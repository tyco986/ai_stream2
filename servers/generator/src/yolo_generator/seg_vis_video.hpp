#pragma once

#include "base_generator/vis_video.hpp"

// Generate YOLO segmentation video pipeline YAML with OSD and one mp4 per input.
//
// Drawer is segfadedrawerwithtracker. OSD displays instance masks.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → detcollector → dslogger → segfadedrawerwithtracker → nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → nvosdbin{N}
//         │     → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
class SegVisVideoGenerator : public VisVideoGenerator {
public:
    SegVisVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path filesinkOutput,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
    void add() override;
    void link() override;
};
