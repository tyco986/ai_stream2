#pragma once

#include "base_generator/base_rtsp.hpp"

// Generate YOLO segmentation RTSP pipeline (headless, ends at fakesink).
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → tee_msg
//           ─┬→ nvstreamdemux → queue_demux{N} → detlogger{N} → fakesink{N}
//           └→ queue_msg → nvmsgconv → nvmsgbroker
class SegRtspGenerator : public BaseRtspGenerator {
public:
    SegRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
