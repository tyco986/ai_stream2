#pragma once

#include "base_generator/vis_sahi_pose_rtsp.hpp"

// Generate YOLO SAHI pose RTSP pipeline with OSD preview sink.
//
// Postprocess element is nvsahipostprocesspose. Drawer is posefadedrawer.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocesspose
//           → boxsnapshot → nvtracker → nvdsanalytics → tee_msg
//             ─┬→ nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → posefadedrawer{N} → nvosdbin{N}
//             │     → queue_enc{N} → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → rtspclientsink{N}
//             └→ queue_msg → nvmsgconv → nvmsgbroker
class PoseSahiVisRtspGenerator : public VisSahiPoseRtspGenerator {
public:
    PoseSahiVisRtspGenerator(
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
