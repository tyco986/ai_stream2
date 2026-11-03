#pragma once

#include "base_generator/base_rtsp.hpp"
#include "stgcnpp_yolo_pose_generator/stgcnpp_yolo_pose.hpp"

#include <optional>

// Generate ST-GCN++ YOLO-pose RTSP pipeline (headless).
//
// PGIE is YOLO pose. SGIE is ST-GCN++ through nvdspreprocess.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvbboxsnapshot → nvtracker → nvdspreprocess → sgie1
//         → nvstgcnppactionlabel → nvdsanalytics → tee_msg
//           ─┬→ nvstreamdemux → queue_demux{N} → nvstgcnpplogger{N} → fakesink{N}
//           └→ queue_msg → nvmsgconv → nvmsgbroker
class StgcnppYoloPoseRtspGenerator : public BaseRtspGenerator {
public:
    StgcnppYoloPoseRtspGenerator(
        std::string pipelineName,
        nlohmann::json streams,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json stgcnpp,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void applyPgieConfig() override;
    void initParams() override;
    void applySavePaths(YAML::Node pipelineDocument, const std::filesystem::path& configSaveDir) override;
    void add() override;
    void link() override;

private:
    nlohmann::json stgcnpp;
    std::optional<StgcnppYoloPose> model;
};
