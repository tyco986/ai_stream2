#pragma once

#include "base_generator/event_video.hpp"

// Generate YOLO detection video pipeline for event alert + nvcapturer dump.
class DetVideoPresenceGenerator : public EventVideoGenerator {
public:
    DetVideoPresenceGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    void applyPgieConfig() override;
};
