#pragma once

#include "base_generator/vis_rtsp.hpp"

// Generate YOLO segmentation RTSP pipeline with OSD preview sink.
//
// Drawer is segfadedrawer. OSD does not display instance masks.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → tee_msg
//           ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → segfadedrawer{N} → nvosdbin{N}
//           │                → queue_enc{N} → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//           └→ queue_msg → nvmsgconv → nvmsgbroker
class SegVisRtspGenerator : public VisRtspGenerator {
public:
    SegVisRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
    void add() override;
    void link() override;
};
