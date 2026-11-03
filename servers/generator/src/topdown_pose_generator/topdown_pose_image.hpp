#pragma once

#include "base_generator/base_image.hpp"
#include "topdown_pose_generator/topdown_pose.hpp"

#include <optional>

// Generate topdown-pose image pipeline YAML.
//
// PGIE detects people. SGIE is RTMPose through nvdspreprocess. Drawer is nvposefadedrawer.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvdspreprocess_rtmpose → sgie0 → nvrtmposepostprocess
//         → nvdsanalytics → tee_msg
//         ├→ nvposefadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue{N} → capsfilter{N} → nvdetlogger{N} → nvjpegenc{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
class TopdownPoseImageGenerator : public BaseImageGenerator {
public:
    TopdownPoseImageGenerator(
        std::string pipelineName,
        nlohmann::json images,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sgie,
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
