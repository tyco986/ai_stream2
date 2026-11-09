#pragma once

#include "base_generator/vis_event_rtsp.hpp"

// Generate YOLO detection RTSP pipeline for event alert + nvcapturer dump.
// Per-stream branches tee raw/vis capturers and continue encode to rtspclientsink.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → presenceeventcoder → tee_msg
//       ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → tee_raw{N}
//       │     ─┬→ queue_raw{N} → nvvideoconvert_raw{N} → capsfilter_raw{N} → rawcapturer{N} → fakesink_raw{N}
//       │     └→ queue_osd{N} → nvvideoconvert_osd{N} → detfadedrawerwithtracker{N} → nvosdbin{N} → tee_vis{N}
//       │           ─┬→ queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → viscapturer{N} → fakesink_vis{N}
//       │           └→ queue_enc{N} → presencelogger{N} → nvvideoconvert_enc{N} → capsfilter_enc{N}
//       │                 → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//       └→ queue_msg → nvmsgconv → nvmsgbroker
class DetVisRtspPresenceGenerator : public VisEventRtspGenerator {
public:
    DetVisRtspPresenceGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    void applyPgieConfig() override;
};
