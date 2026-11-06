#pragma once

#include "base_generator/base_video.hpp"

// Generate YOLO detection video pipeline YAML (headless, ends at fakesink).
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ detlogger → fakesink
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class DetVideoGenerator : public BaseVideoGenerator {
public:
    DetVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
