#pragma once

#include "base_generator/sahi_rtsp.hpp"

// Generate YOLO SAHI detection RTSP pipeline (headless, ends at fakesink).
//
// mux batch size is the stream count. pgie batch size is the SAHI tile count times the stream count.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//           → boxsnapshot → nvtracker → nvdsanalytics → nvstreamdemux → queue_demux{N} → detlogger{N} → fakesink{N}
class DetSahiRtspGenerator : public SahiRtspGenerator {
public:
    DetSahiRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
