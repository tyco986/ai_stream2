#pragma once

#include "base_generator/vis_rtsp.hpp"

// Generate YOLO detection RTSP pipeline with OSD preview sink.
//
// Drawer is nvdetfadedrawer. OSD displays bbox and text.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//           ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → nvdetfadedrawer{N} → nvosdbin{N}
//           │                → queue_enc{N} → nvdetlogger{N} → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//           └→ queue_msg → nvmsgconv → nvmsgbroker
class DetVisRtspGenerator : public VisRtspGenerator {
public:
    DetVisRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
