#pragma once

#include "base_generator/event_video.hpp"

// Generate YOLO event video pipeline with one mp4 per input.
//
// Capture branches stay on the batched stream. Encode is demuxed per video.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → nvpresencecoder → tee_msg
//       └→ … → tee_vis
//             ─┬→ queue_vis → nvviscapturer → fakesink_vis
//             └→ queue_enc → nvpresencelogger → nvstreamdemux
//                   → queue_demux{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
class VisEventVideoGenerator : public EventVideoGenerator {
public:
    VisEventVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json eventCoder,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt,
        std::optional<nlohmann::json> capturer = std::nullopt);

    void initVideos() override;
    void initParams() override;

    std::filesystem::path output;

protected:
    void appendEncode(int gpuId) override;
    void linkEncode(YAML::Node edges) override;
};
