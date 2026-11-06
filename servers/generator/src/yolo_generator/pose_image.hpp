#pragma once

#include "base_generator/base_image.hpp"

// Generate YOLO pose image pipeline YAML.
//
// Drawer is posefadedrawer. OSD displays bbox and text.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvdsanalytics → tee_msg
//         ├→ posefadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue{N} → capsfilter{N} → detlogger{N} → nvjpegenc{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class PoseImageGenerator : public BaseImageGenerator {
public:
    PoseImageGenerator(
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
