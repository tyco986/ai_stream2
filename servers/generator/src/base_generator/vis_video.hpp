#pragma once

#include "base_generator/base_video.hpp"

#include <filesystem>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

// Generate YOLO video pipeline YAML with OSD and one mp4 per input.
//
// Each file is demuxed before OSD and encoded on its own branch, so a short
// file finalizes its mp4 on EOS while the longer files keep running.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → detfadedrawer{N} → nvosdbin{N}
//         │     → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
class VisVideoGenerator : public BaseVideoGenerator {
public:
    VisVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void initVideos() override;
    void initParams() override;
    void add() override;
    void link() override;

    std::filesystem::path output;
};
