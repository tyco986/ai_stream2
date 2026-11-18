#pragma once

#include "base_generator/base_image.hpp"

// Generate YOLO detection image pipeline YAML.
//
// Drawer is detfadedrawer. OSD displays bbox and text.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvdsanalytics → detcollector → dslogger → detfadedrawer → nvstreamdemux → queue_demux{N} → nvosdbin{N} → nvvideoconvert{N}
//         │     → capsfilter{N} → nvjpegenc{N} → filesink{N}
class DetImageGenerator : public BaseImageGenerator {
public:
    DetImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path filesinkOutput,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
