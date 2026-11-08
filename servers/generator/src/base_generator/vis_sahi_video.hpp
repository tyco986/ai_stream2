#pragma once

#include "base_generator/sahi_video.hpp"

#include <filesystem>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

// Generate YOLO SAHI video pipeline YAML with OSD and one mp4 per input.
//
// mux batch size is the number of videos. pgie batch size is the SAHI tile count.
//
// Topology::
//
//     nvurisrcbin{N} → nvstreammux → nvsahipreprocess → pgie → queue_sahi → nvsahipostprocess
//         → boxsnapshot → nvtracker → nvdsanalytics → nvstreamdemux → queue_demux{N} → nvvideoconvert{N} → detfadedrawerwithtracker{N} → nvosdbin{N}
//         │     → detlogger{N} → nvv4l2h264enc{N} → h264parse{N} → mp4mux{N} → filesink{N}
class VisSahiVideoGenerator : public SahiVideoGenerator {
public:
    VisSahiVideoGenerator(
        std::string pipelineName,
        nlohmann::json videos,
        std::filesystem::path output,
        std::optional<nlohmann::json> analyzer,
        nlohmann::json pgie,
        nlohmann::json sahi,
        std::optional<nlohmann::json> tracker = std::nullopt,
        std::optional<nlohmann::json> logger = std::nullopt,
        std::optional<nlohmann::json> drawer = std::nullopt);

    void initVideos() override;
    void initParams() override;
    void add() override;
    void link() override;

    std::filesystem::path output;

protected:
    virtual std::string visDrawerElement() const;
    virtual YAML::Node visDrawerProperties(const nlohmann::json& drawer);
};
