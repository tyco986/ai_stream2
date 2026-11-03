#pragma once

#include "base_generator/vis_sahi_rtsp.hpp"

// Generate YOLO SAHI pose RTSP pipeline with OSD preview sink.
//
// Postprocess element is nvsahipostprocess_pose. Drawer is nvposefadedrawer.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess_pose
//           → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//             ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → nvposefadedrawer{N} → nvosdbin{N}
//             │     → queue_enc{N} → nvdetlogger{N} → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//             └→ queue_msg → nvmsgconv → nvmsgbroker
class VisSahiPoseRtspGenerator : public VisSahiRtspGenerator {
public:
    VisSahiPoseRtspGenerator(
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
    std::string visDrawerElement() const override;
    YAML::Node visDrawerProperties(const nlohmann::json& drawer) override;
};
