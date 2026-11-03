#pragma once

#include "base_generator/base_video.hpp"

// Generate YOLO segmentation video pipeline YAML (headless, ends at fakesink).
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvbboxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ nvdetlogger → fakesink
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class SegVideoGenerator : public BaseVideoGenerator {
public:
    SegVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
