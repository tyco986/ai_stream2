#pragma once

#include "base_generator/vis_event_rtsp.hpp"

// Generate YOLO detection RTSP pipeline for event alert + nvcapturer dump.
// Per-stream branches tee raw/vis capturers and continue encode to rtspclientsink.
class DetVisRtspPresenceGenerator : public VisEventRtspGenerator {
public:
    DetVisRtspPresenceGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    void applyPgieConfig() override;
};
