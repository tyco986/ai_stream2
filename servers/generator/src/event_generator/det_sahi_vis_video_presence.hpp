#pragma once

#include "base_generator/vis_event_sahi_video.hpp"

// Generate YOLO SAHI vis-video pipeline for event alert + nvcapturer dump.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → boxsnapshot → nvtracker → nvdsanalytics → presenceeventcoder → tee_msg
//       ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → tee_raw{N}
//       │     ─┬→ queue_raw{N} → nvvideoconvert_raw{N} → capsfilter_raw{N} → rawcapturer{N} → fakesink_raw{N}
//       │     └→ queue_osd{N} → nvvideoconvert_osd{N} → detfadedrawerwithtracker{N} → nvosdbin{N} → tee_vis{N}
//       │           ─┬→ queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → viscapturer{N} → fakesink_vis{N}
//       │           └→ queue_enc{N} → presencelogger{N} → nvvideoconvert_enc{N} → capsfilter_enc{N}
//       │                 → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
//       └→ queue_msg → nvmsgconv → nvmsgbroker
class DetSahiVisVideoPresenceGenerator : public VisEventSahiVideoGenerator {
public:
    DetSahiVisVideoPresenceGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    void applyPgieConfig() override;
};
