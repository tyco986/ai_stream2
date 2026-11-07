#pragma once

#include "base_generator/sahi_image.hpp"

// Generate YOLO SAHI segmentation image pipeline YAML.
//
// Drawer is segfadedrawer. OSD displays instance masks.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → nvdsanalytics → nvstreamdemux → queue_demux{N} → segfadedrawer{N} → nvosdbin{N} → nvvideoconvert{N}
//         │     → capsfilter{N} → detlogger{N} → nvjpegenc{N} → filesink{N}
class SegSahiImageGenerator : public SahiImageGenerator {
public:
    SegSahiImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
    void add() override;
    void link() override;
};
