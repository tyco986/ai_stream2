#pragma once

#include "base_generator/base_image.hpp"

// Generate YOLO segmentation image pipeline YAML.
//
// Drawer is segfadedrawer. OSD displays instance masks.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvdsanalytics → detcollector → dslogger → segfadedrawer → nvstreamdemux → queue_demux{N} → nvosdbin{N} → nvvideoconvert{N}
//         │     → capsfilter{N} → nvjpegenc{N} → filesink{N}
class SegImageGenerator : public BaseImageGenerator {
public:
    SegImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path filesinkOutput,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
    void add() override;
    void link() override;
};
