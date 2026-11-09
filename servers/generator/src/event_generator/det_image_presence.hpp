#pragma once

#include "base_generator/event_image.hpp"

// Generate YOLO detection image pipeline for event alert + nvcapturer dump.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvdsanalytics → presenceeventcoder → tee_msg
//       ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → tee_raw{N}
//       │     ─┬→ queue_raw{N} → nvvideoconvert_raw{N} → capsfilter_raw{N} → rawcapturer{N} → fakesink_raw{N}
//       │     └→ queue_osd{N} → nvvideoconvert_osd{N} → detfadedrawer{N} → nvosdbin{N} → tee_vis{N}
//       │           ─┬→ queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → viscapturer{N} → fakesink_vis{N}
//       │           └→ queue_enc{N} → presencelogger{N} → nvvideoconvert_enc{N} → capsfilter_jpeg{N}
//       │                 → nvjpegenc{N} → filesink{N}
//       └→ queue_msg → nvmsgconv → nvmsgbroker
class DetImagePresenceGenerator : public EventImageGenerator {
public:
    DetImagePresenceGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    void applyPgieConfig() override;
};
