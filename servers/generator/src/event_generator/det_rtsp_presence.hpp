#pragma once

#include "base_generator/event_rtsp.hpp"

// Generate YOLO detection RTSP pipeline for event alert + nvcapturer dump.
// No RTSP preview sink: capture ends at capturer + fakesink.
class DetRtspPresenceGenerator : public EventRtspGenerator {
public:
    DetRtspPresenceGenerator(
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
