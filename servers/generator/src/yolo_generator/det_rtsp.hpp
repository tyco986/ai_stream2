#pragma once

#include "base_generator/base_rtsp.hpp"

// Generate YOLO detection RTSP pipeline (headless, ends at fakesink).
//
// Set analyzer to null to disable nvdsanalytics rules. Set tracker to null to skip nvtracker.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → nvstreamdemux → queue_demux{N} → detlogger{N} → fakesink{N}
class DetRtspGenerator : public BaseRtspGenerator {
public:
    DetRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
