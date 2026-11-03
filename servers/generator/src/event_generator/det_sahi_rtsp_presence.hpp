#pragma once

#include "base_generator/event_sahi_rtsp.hpp"

// Generate YOLO SAHI detection RTSP pipeline for event alert + nvcapturer dump.
// No RTSP preview sink: capture ends at capturer + fakesink.
class DetSahiRtspPresenceGenerator : public EventSahiRtspGenerator {
public:
    DetSahiRtspPresenceGenerator(
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
