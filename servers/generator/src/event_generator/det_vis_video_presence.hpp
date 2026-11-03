#pragma once

#include "base_generator/vis_event_video.hpp"

// Generate YOLO detection vis-video pipeline for event alert + nvcapturer dump.
class DetVisVideoPresenceGenerator : public VisEventVideoGenerator {
public:
    DetVisVideoPresenceGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    void applyPgieConfig() override;
};
