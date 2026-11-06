#pragma once

#include "base_generator/sahi_image.hpp"

// Generate YOLO SAHI pose image pipeline YAML.
//
// Postprocess element is nvsahipostprocesspose. Drawer is posefadedrawer.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocesspose
//         → nvdsanalytics → tee_msg
//         ├→ posefadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue_demux{N} → capsfilter{N} → detlogger{N} → nvjpegenc{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class SahiPoseImageGenerator : public SahiImageGenerator {
public:
    SahiPoseImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

protected:
    YAML::Node sahiPostprocessProperties(const nlohmann::json& postprocess) override;
    std::string visDrawerElement() const override;
    YAML::Node visDrawerProperties(const nlohmann::json& drawer) override;
};
