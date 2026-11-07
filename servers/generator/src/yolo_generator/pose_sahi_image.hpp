#pragma once

#include "base_generator/sahi_pose_image.hpp"

// Generate YOLO SAHI pose image pipeline YAML.
//
// Postprocess element is nvsahipostprocesspose. Drawer is posefadedrawer.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocesspose
//         → nvdsanalytics → nvstreamdemux → queue_demux{N} → posefadedrawer{N} → nvosdbin{N} → nvvideoconvert{N}
//         │     → capsfilter{N} → detlogger{N} → nvjpegenc{N} → filesink{N}
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
