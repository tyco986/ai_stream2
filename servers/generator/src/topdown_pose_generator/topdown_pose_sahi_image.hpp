#pragma once

#include "base_generator/sahi_image.hpp"
#include "topdown_pose_generator/topdown_pose.hpp"

#include <optional>

// Generate topdown-pose SAHI image pipeline YAML.
//
// mux batch size is the image count. pgie batch size is the SAHI tile count.
// PGIE detects people. SGIE is RTMPose through nvdspreprocess. Drawer is posefadedrawer.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → nvdspreprocess_rtmpose → sgie0 → nvrtmposepostprocess → nvdsanalytics → tee_msg
//         ├→ posefadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue_demux{N} → capsfilter{N} → detlogger{N} → nvjpegenc{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class TopdownPoseSahiImageGenerator : public SahiImageGenerator {
public:
    TopdownPoseSahiImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
    void initParams() override;
    void applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) override;
    void add() override;
    void link() override;

private:
    nlohmann::json sgie;
    std::optional<TopdownPose> pose;
};
