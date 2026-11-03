#pragma once

#include "base_generator/sahi_rtsp.hpp"

// Generate YOLO SAHI pose RTSP pipeline (headless, ends at fakesink).
//
// Postprocess element is nvsahipostprocess_pose.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess_pose
//           → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//             ─┬→ nvstreamdemux → queue_demux{N} → nvdetlogger{N} → fakesink{N}
//             └→ queue_msg → nvmsgconv → nvmsgbroker
class SahiPoseRtspGenerator : public SahiRtspGenerator {
public:
    SahiPoseRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

protected:
    YAML::Node sahiPostprocessProperties(const nlohmann::json& postprocess) override;
};
