#pragma once

#include "base_generator/base_image.hpp"

// Generate YOLO segmentation image pipeline YAML.
//
// Drawer is nvsegfadedrawer. OSD displays instance masks.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvdsanalytics → tee_msg
//         ├→ nvsegfadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue{N} → capsfilter{N} → nvdetlogger{N} → nvjpegenc{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class SegImageGenerator : public BaseImageGenerator {
public:
    SegImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
    void add() override;
    void link() override;
};
