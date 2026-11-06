#pragma once

#include "base_generator/vis_sahi_rtsp.hpp"

// Generate YOLO SAHI segmentation RTSP pipeline with OSD preview sink.
//
// Drawer is segfadedrawer. OSD does not display instance masks.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//           → boxsnapshot → nvtracker → nvdsanalytics → tee_msg
//             ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → segfadedrawer{N} → nvosdbin{N}
//             │     → queue_enc{N} → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//             └→ queue_msg → nvmsgconv → nvmsgbroker
class SegSahiVisRtspGenerator : public VisSahiRtspGenerator {
public:
    SegSahiVisRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
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
