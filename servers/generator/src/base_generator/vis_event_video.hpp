#pragma once

#include "base_generator/event_video.hpp"

// Generate YOLO event video pipeline with one mp4 per input.
//
// OSD runs after demux, one surface per stream.
//
// Topology::
//
//     nvosdbin{N} → tee_vis{N}
//       ─┬→ queue_vis{N} → nvvideoconvert_vis{N} → capsfilter_vis{N} → viscapturer{N} → fakesink_vis{N}
//       └→ queue_enc{N} → presencelogger{N} → nvvideoconvert_enc{N} → capsfilter_enc{N}
//         → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
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
    void appendAfterOsd(int index, int gpuId) override;
    void linkAfterOsd(YAML::Node edges, int index) override;
};
