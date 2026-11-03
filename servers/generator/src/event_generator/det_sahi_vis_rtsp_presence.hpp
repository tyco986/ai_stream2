#pragma once

#include "base_generator/vis_event_sahi_rtsp.hpp"

// Generate YOLO SAHI detection RTSP pipeline for event alert + nvcapturer dump.
// Per-stream branches tee raw/vis capturers and continue encode to rtspclientsink.
class DetSahiVisRtspPresenceGenerator : public VisEventSahiRtspGenerator {
public:
    DetSahiVisRtspPresenceGenerator(
        std::string pipelineName,
        nlohmann::json streams,
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
