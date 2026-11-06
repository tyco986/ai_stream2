#pragma once

#include "base_generator/base_video.hpp"

#include <filesystem>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

// Generate YOLO video pipeline YAML with OSD and one mp4 per input.
//
// Each file is demuxed after OSD and encoded on its own branch, so a short
// file finalizes its mp4 on EOS while the longer files keep running.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → pgie → boxsnapshot → nvtracker → nvdsanalytics → tee_msg
//         ├→ detfadedrawer → nvosdbin → nvvideoconvert → nvstreamdemux
//         │     → queue{N} → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
//         └→ queue_msg → nvmsgconv → nvmsgbroker
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
