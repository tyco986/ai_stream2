#pragma once

#include "base_generator/sahi_video.hpp"

// Generate YOLO SAHI segmentation video pipeline YAML (headless, ends at fakesink).
//
// mux batch size is the number of videos. pgie batch size is the SAHI tile count.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → boxsnapshot → nvtracker → nvdsanalytics → detlogger → fakesink
class SegSahiVideoGenerator : public SahiVideoGenerator {
public:
    SegSahiVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
