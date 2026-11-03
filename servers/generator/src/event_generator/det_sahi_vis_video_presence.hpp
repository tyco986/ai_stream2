#pragma once

#include "base_generator/vis_event_sahi_video.hpp"

// Generate YOLO SAHI vis-video pipeline for event alert + nvcapturer dump.
class DetSahiVisVideoPresenceGenerator : public VisEventSahiVideoGenerator {
public:
    DetSahiVisVideoPresenceGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
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
