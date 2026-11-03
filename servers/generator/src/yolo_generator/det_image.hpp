#pragma once

#include "base_generator/base_image.hpp"

// Generate YOLO detection image pipeline YAML.
//
// Drawer is nvdetfadedrawer. OSD displays bbox and text.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvdsanalytics → tee_msg
//         ├→ nvdetfadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue{N} → capsfilter{N} → nvdetlogger{N} → nvjpegenc{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class DetImageGenerator : public BaseImageGenerator {
public:
    DetImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
