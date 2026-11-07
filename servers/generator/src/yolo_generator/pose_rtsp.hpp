#pragma once

#include "base_generator/base_rtsp.hpp"

// Generate YOLO pose RTSP pipeline (headless, ends at fakesink).
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → nvstreamdemux → queue_demux{N} → detlogger{N} → fakesink{N}
class PoseRtspGenerator : public BaseRtspGenerator {
public:
    PoseRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
