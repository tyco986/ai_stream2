#pragma once

#include "base_generator/sahi_pose_image.hpp"

// Generate YOLO SAHI pose image pipeline YAML.
//
// Postprocess element is nvsahipostprocess_pose. Drawer is nvposefadedrawer.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess_pose
//         → nvdsanalytics → tee_msg
//         ├→ nvposefadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue_demux{N} → capsfilter{N} → nvdetlogger{N} → nvjpegenc{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class PoseSahiImageGenerator : public SahiPoseImageGenerator {
public:
    PoseSahiImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
};
