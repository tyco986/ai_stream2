#pragma once

#include "base_generator/event_sahi_rtsp.hpp"

// Generate YOLO SAHI detection RTSP pipeline for event alert + nvcapturer dump.
// No RTSP preview sink: capture ends at capturer + fakesink.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → boxsnapshot → nvtracker → nvdsanalytics → presenceeventcoder → presencecollector → dslogger → tee_msg
//       ─┬→ detfadedrawerwithtracker → nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → tee_raw{N}
//       │     ─┬→ queue_raw{N} → nvvideoconvert_raw{N} → capsfilter_raw{N} → rawcapturer{N} → fakesink_raw{N}
//       │     └→ queue_osd{N} → nvvideoconvert_osd{N} → nvosdbin{N}
//       │           → queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → viscapturer{N} → fakesink_vis{N}
//       └→ queue_msg → nvmsgconv → nvmsgbroker
class DetSahiRtspPresenceGenerator : public EventSahiRtspGenerator {
public:
    DetSahiRtspPresenceGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::filesystem::path capturerOutput,
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
