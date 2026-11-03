#pragma once

#include "base_generator/base_rtsp.hpp"
#include "topdown_pose_generator/topdown_pose.hpp"

#include <optional>

// Generate topdown-pose RTSP pipeline (headless).
//
// PGIE detects people. SGIE is RTMPose through nvdspreprocess.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvbboxsnapshot → nvtracker → nvdspreprocess_rtmpose → sgie0
//         → nvrtmposepostprocess → nvdsanalytics → tee_msg
//           ─┬→ nvstreamdemux → queue_demux{N} → nvdetlogger{N} → fakesink{N}
//           └→ queue_msg → nvmsgconv → nvmsgbroker
class TopdownPoseRtspGenerator : public BaseRtspGenerator {
public:
    TopdownPoseRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
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
